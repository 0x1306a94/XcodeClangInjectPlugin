#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace llvm;
namespace kk {

class CodeStyleConsumer : public ASTConsumer {
    CompilerInstance &CI;
    std::unique_ptr<clang::ASTConsumer> codegenConsumer;

  public:
    CodeStyleConsumer(CompilerInstance &CI_, std::unique_ptr<clang::ASTConsumer> codegenConsumer_)
        : CI(CI_)
        , codegenConsumer(std::move(codegenConsumer_)) {

        const FrontendOptions &FEOpts = CI.getFrontendOpts();
        errs() << "plugin args: " << FEOpts.PluginArgs.size() << '\n';
    }

    /// Initialize - This is called to initialize the consumer, providing the
    /// ASTContext.
    virtual void Initialize(ASTContext &Context) override {
        codegenConsumer->Initialize(Context);
    }

    /// HandleTopLevelDecl - Handle the specified top-level declaration.  This is
    /// called by the parser to process every top-level Decl*.
    ///
    /// \returns true to continue parsing, or false to abort parsing.
    virtual bool HandleTopLevelDecl(DeclGroupRef D) override {
        return codegenConsumer->HandleTopLevelDecl(D);
    }

    /// This callback is invoked each time an inline (method or friend)
    /// function definition in a class is completed.
    virtual void HandleInlineFunctionDefinition(FunctionDecl *D) override {
        codegenConsumer->HandleInlineFunctionDefinition(D);
    }

    /// HandleInterestingDecl - Handle the specified interesting declaration. This
    /// is called by the AST reader when deserializing things that might interest
    /// the consumer. The default implementation forwards to HandleTopLevelDecl.
    virtual void HandleInterestingDecl(DeclGroupRef D) override {
        codegenConsumer->HandleInterestingDecl(D);
    }

    /// HandleTranslationUnit - This method is called when the ASTs for entire
    /// translation unit have been parsed.
    virtual void HandleTranslationUnit(ASTContext &Ctx) override {
        codegenConsumer->HandleTranslationUnit(Ctx);
    }

    /// HandleTagDeclDefinition - This callback is invoked each time a TagDecl
    /// (e.g. struct, union, enum, class) is completed.  This allows the client to
    /// hack on the type, which can occur at any point in the file (because these
    /// can be defined in declspecs).
    virtual void HandleTagDeclDefinition(TagDecl *D) override {
        codegenConsumer->HandleTagDeclDefinition(D);
    }

    /// This callback is invoked the first time each TagDecl is required to
    /// be complete.
    virtual void HandleTagDeclRequiredDefinition(const TagDecl *D) override {
        codegenConsumer->HandleTagDeclRequiredDefinition(D);
    }

    /// Invoked when a function is implicitly instantiated.
    /// Note that at this point it does not have a body, its body is
    /// instantiated at the end of the translation unit and passed to
    /// HandleTopLevelDecl.
    virtual void HandleCXXImplicitFunctionInstantiation(FunctionDecl *D) override {
        codegenConsumer->HandleCXXImplicitFunctionInstantiation(D);
    }

    /// Handle the specified top-level declaration that occurred inside
    /// and ObjC container.
    /// The default implementation ignored them.
    virtual void HandleTopLevelDeclInObjCContainer(DeclGroupRef D) override {
        codegenConsumer->HandleTopLevelDeclInObjCContainer(D);
    }

    /// Handle an ImportDecl that was implicitly created due to an
    /// inclusion directive.
    /// The default implementation passes it to HandleTopLevelDecl.
    virtual void HandleImplicitImportDecl(ImportDecl *D) override {
        codegenConsumer->HandleImplicitImportDecl(D);
    }

    /// CompleteTentativeDefinition - Callback invoked at the end of a translation
    /// unit to notify the consumer that the given tentative definition should be
    /// completed.
    ///
    /// The variable declaration itself will be a tentative
    /// definition. If it had an incomplete array type, its type will
    /// have already been changed to an array of size 1. However, the
    /// declaration remains a tentative definition and has not been
    /// modified by the introduction of an implicit zero initializer.
    virtual void CompleteTentativeDefinition(VarDecl *D) override {
        codegenConsumer->CompleteTentativeDefinition(D);
    }

    /// CompleteExternalDeclaration - Callback invoked at the end of a translation
    /// unit to notify the consumer that the given external declaration should be
    /// completed.
    virtual void CompleteExternalDeclaration(VarDecl *D) override {
        codegenConsumer->CompleteExternalDeclaration(D);
    }

    /// Callback invoked when an MSInheritanceAttr has been attached to a
    /// CXXRecordDecl.
    virtual void AssignInheritanceModel(CXXRecordDecl *RD) override {
        codegenConsumer->AssignInheritanceModel(RD);
    }

    /// HandleCXXStaticMemberVarInstantiation - Tell the consumer that this
    // variable has been instantiated.
    virtual void HandleCXXStaticMemberVarInstantiation(VarDecl *D) override {
        codegenConsumer->HandleCXXStaticMemberVarInstantiation(D);
    }

    /// Callback involved at the end of a translation unit to
    /// notify the consumer that a vtable for the given C++ class is
    /// required.
    ///
    /// \param RD The class whose vtable was used.
    virtual void HandleVTable(CXXRecordDecl *RD) override {
        codegenConsumer->HandleVTable(RD);
    }

    /// If the consumer is interested in entities getting modified after
    /// their initial creation, it should return a pointer to
    /// an ASTMutationListener here.
    virtual ASTMutationListener *GetASTMutationListener() override {
        return codegenConsumer->GetASTMutationListener();
    }

    /// If the consumer is interested in entities being deserialized from
    /// AST files, it should return a pointer to a ASTDeserializationListener here
    virtual ASTDeserializationListener *GetASTDeserializationListener() override {
        return codegenConsumer->GetASTDeserializationListener();
    }

    /// PrintStats - If desired, print any statistics.
    virtual void PrintStats() override {
        codegenConsumer->PrintStats();
    }

    /// This callback is called for each function if the Parser was
    /// initialized with \c SkipFunctionBodies set to \c true.
    ///
    /// \return \c true if the function's body should be skipped. The function
    /// body may be parsed anyway if it is needed (for instance, if it contains
    /// the code completion point or is constexpr).
    virtual bool shouldSkipFunctionBody(Decl *D) override {
        return codegenConsumer->shouldSkipFunctionBody(D);
    }
};
}  // namespace kk

extern "C" {
std::unique_ptr<clang::ASTConsumer> plugin_entry_create_consumer(std::unique_ptr<clang::ASTConsumer> codegenConsumer, clang::CompilerInstance *CI, llvm::StringRef InFile) {
    if (!CI) {
        return codegenConsumer;
    }
    return std::make_unique<kk::CodeStyleConsumer>(*CI, std::move(codegenConsumer));
}
}
