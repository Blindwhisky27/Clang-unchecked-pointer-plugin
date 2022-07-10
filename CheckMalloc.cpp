#include <unordered_set>
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/FileManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"
#include <string>
using namespace clang;

class CheckMallocVisitor : public RecursiveASTVisitor<CheckMallocVisitor>
{
private:
	ASTContext *context;
	CompilerInstance &instance;

	DiagnosticsEngine &d;

	unsigned int warningID;
	bool isInHeader(IfStmt *decl)
	{
		auto loc = decl->getBeginLoc();
		auto floc = context->getFullLoc(loc);
		if (floc.isInSystemHeader())
			return true;
		auto entry = floc.getFileEntry()->getName();
		if (entry.endswith(".h") || entry.endswith(".hpp"))
		{
			return true;
		}
		return false;
	}

	bool checkMalloc(CallExpr *S, std::string name)
	{
		return (name.compare("malloc") == 0);
	}
	bool checkIfStmt(IfStmt *s, ASTContext *context)
	{
		Expr *e = s->getCond();
		CharSourceRange cha = Lexer::getAsCharRange(e->getSourceRange(), context->getSourceManager(), context->getLangOpts());
		llvm::StringRef text = Lexer::getSourceText(cha, context->getSourceManager(), context->getLangOpts());

		return text.contains("!= NULL");
	}

public:
	explicit CheckMallocVisitor(ASTContext *context, CompilerInstance &instance) : context(context), instance(instance), d(instance.getDiagnostics())
	{
		warningID = d.getCustomDiagID(DiagnosticsEngine::Warning,
									  "Unchecked pointer: '%0'");
	}
	bool isMallocCalled = false;
	bool isPtrChecked = false;

	virtual bool VisitCallExpr(CallExpr *S)
	{
		isMallocCalled = checkMalloc(S, S->getCalleeDecl()->getAsFunction()->getNameAsString());
		return true;
	}
	virtual bool VisitIfStmt(IfStmt *S)
	{
		isPtrChecked = checkIfStmt(S, context);
		return true;
	}
	virtual bool VisitStmt(Stmt *S)
	{
		if (isMallocCalled & !isPtrChecked & std::string(S->getStmtClassName()).compare("BinaryOperator") == 0)
		{
			auto loc = context->getFullLoc(S->getBeginLoc());
			d.Report(loc, warningID) << "Check pointer for NULL before using";
			isMallocCalled = false;
			isPtrChecked = false;
		}
		return true;
	}
};

class CheckMallocConsumer : public ASTConsumer
{
	CompilerInstance &instance;
	CheckMallocVisitor visitor;

public:
	CheckMallocConsumer(CompilerInstance &instance)
		: instance(instance), visitor(&instance.getASTContext(), instance) {}

	virtual void HandleTranslationUnit(ASTContext &context) override
	{
		visitor.TraverseDecl(context.getTranslationUnitDecl());
	}
};

class CheckMallocAction : public PluginASTAction
{
protected:
	virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &instance, llvm::StringRef) override
	{
		return std::make_unique<CheckMallocConsumer>(instance);
	}

	virtual bool ParseArgs(const CompilerInstance &Compiler, const std::vector<std::string> &) override
	{
		return true;
	}

	virtual PluginASTAction::ActionType getActionType() override
	{
		return PluginASTAction::AddAfterMainAction;
	}
};

static FrontendPluginRegistry::Add<CheckMallocAction> CheckMalloc("CheckMalloc", "Warn against unchecked pointers");
