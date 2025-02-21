#include <clang/Basic/Diagnostic.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>

#include <filesystem>

#include "dobby.h"

#include <mach-o/dyld.h>

using namespace std;
using namespace llvm;
using namespace clang;

namespace fs = std::filesystem;

using CreateFromArgsImplFunc = bool (*)(CompilerInvocation &Res, ArrayRef<const char *> CommandLineArgs, DiagnosticsEngine &Diags, const char *Argv0);

using GenerateCC1CommandLineFunc = void (*)(CompilerInvocation *CI, SmallVectorImpl<const char *> &Args, CompilerInvocation::StringAllocator SA);

using CodeGenActionCreateASTConsumerFunc = std::unique_ptr<clang::ASTConsumer> (*)(clang::CodeGenAction *AC, clang::CompilerInstance *CI, llvm::StringRef InFile);
using PluginEntryFunc = std::unique_ptr<clang::ASTConsumer> (*)(std::unique_ptr<clang::ASTConsumer> codegenConsumer, clang::CompilerInstance *CI, llvm::StringRef InFile);

CodeGenActionCreateASTConsumerFunc original_CodeGenActionCreateASTConsumerFunc = nullptr;
GenerateCC1CommandLineFunc original_GenerateCC1CommandLineFunc = nullptr;
CreateFromArgsImplFunc original_CreateFromArgsImplFunc = nullptr;
static std::unique_ptr<clang::ASTConsumer> hooked_CodeGenActionCreateASTConsumerFunc(clang::CodeGenAction *AC, clang::CompilerInstance *CI, llvm::StringRef InFile) {

    auto &Diag = CI->getDiagnostics();
    auto executablePath = fs::canonical(_dyld_get_image_name(0));
    auto libPath = executablePath.parent_path().parent_path().append("lib");
    auto pluginPath = libPath.append("CheckerCodeStylePlugin.dylib");
    errs() << "plugin path: " << pluginPath.string() << '\n';
    std::string Error;
    auto lib = sys::DynamicLibrary::getPermanentLibrary(pluginPath.c_str(), &Error);
    if (!lib.isValid()) {
        errs() << "canot load plugin: " << pluginPath << " error: " << Error << '\n';
        Diag.setErrorLimit(0);
        return nullptr;
    }

    auto codegenConsumer = original_CodeGenActionCreateASTConsumerFunc(AC, CI, InFile);
    void *targetAddr = lib.getAddressOfSymbol("plugin_entry_create_consumer");
    if (targetAddr == nullptr) {
        errs() << "canot load plugin: " << pluginPath << " missing symbol: plugin_entry_create_consumer" << '\n';
        Diag.setErrorLimit(0);
        return nullptr;
    }

    PluginEntryFunc entryFunc = reinterpret_cast<PluginEntryFunc>(targetAddr);
    auto consomer = entryFunc(std::move(codegenConsumer), CI, InFile);
    return consomer;
}

static void hooked_GenerateCC1CommandLineFunc(CompilerInvocation *CI, SmallVectorImpl<const char *> &Args, CompilerInvocation::StringAllocator SA) {
    errs() << __FUNCTION__ << '\n';
    original_GenerateCC1CommandLineFunc(CI, Args, SA);
}

static bool hooked_CreateFromArgsImplFunc(CompilerInvocation &Res, ArrayRef<const char *> CommandLineArgs, DiagnosticsEngine &Diags, const char *Argv0) {
    errs() << __FUNCTION__ << '\n';

    std::map<std::string, std::vector<std::string>> pluginArgsMap;
    for (size_t i = 0; i < CommandLineArgs.size(); ++i) {
        std::string arg = CommandLineArgs[i];
        if (arg.find("-plugin-arg-") == 0) {
            // 提取插件名（去掉 -plugin-arg- 前缀）
            std::string pluginName = arg.substr(strlen("-plugin-arg-"));
            // 检查是否有后续参数
            if (i + 1 < CommandLineArgs.size() && CommandLineArgs[i + 1][0] != '-') {
                std::string paramValue = CommandLineArgs[i + 1];
                pluginArgsMap[pluginName].push_back(paramValue);
                errs() << "Found plugin arg for '" << pluginName << "': " << paramValue << '\n';
                i += 1;  // 跳过参数值
            }
        }
    }
    errs() << '\n';
    auto success = original_CreateFromArgsImplFunc(Res, CommandLineArgs, Diags, Argv0);
    if (!success) {
        return success;
    }
    FrontendOptions &FEOpts = Res.getFrontendOpts();
    // 保存到 FrontendOptions 的 Plugins 和 PluginArgs
    std::map<std::string, std::vector<std::string>> backup;
    std::swap(backup, FEOpts.PluginArgs);
    for (const auto &pair : pluginArgsMap) {
        backup.insert_or_assign(pair.first, pair.second);
    }
    std::swap(FEOpts.PluginArgs, backup);


    return success;
}

static __attribute__((__constructor__)) void InjectPlugin(int argc, char *argv[]) {

    auto executablePath = fs::canonical(argv[0]);
    const auto executable = executablePath.c_str();

    errs() << "Applying Clang hook: " << executable << '\n';

    void *targetAddr = DobbySymbolResolver(executable, "__ZN5clang13CodeGenAction17CreateASTConsumerERNS_16CompilerInstanceEN4llvm9StringRefE");
    errs() << "clang::CodeGenAction::CreateASTConsumer: " << targetAddr << " -> " << (void *)hooked_CodeGenActionCreateASTConsumerFunc << '\n';

    void *original_ptr = nullptr;
    DobbyHook(targetAddr, (void *)hooked_CodeGenActionCreateASTConsumerFunc, &original_ptr);
    original_CodeGenActionCreateASTConsumerFunc = reinterpret_cast<CodeGenActionCreateASTConsumerFunc>(original_ptr);

    targetAddr = DobbySymbolResolver(executable, "__ZZNK5clang22CompilerInvocationBase22generateCC1CommandLineERN4llvm15SmallVectorImplIPKcEENS1_12function_refIFS4_RKNS1_5TwineEEEEENKUlSA_E_clESA_");
    errs() << "CompilerInvocation::generateCC1CommandLine: " << targetAddr << " -> " << (void *)hooked_GenerateCC1CommandLineFunc << '\n';

    original_ptr = nullptr;
    DobbyHook(targetAddr, (void *)hooked_GenerateCC1CommandLineFunc, &original_ptr);
    original_GenerateCC1CommandLineFunc = reinterpret_cast<GenerateCC1CommandLineFunc>(original_ptr);

    targetAddr = DobbySymbolResolver(executable, "__ZN5clang18CompilerInvocation18CreateFromArgsImplERS0_N4llvm8ArrayRefIPKcEERNS_17DiagnosticsEngineES5_");
    errs() << "clang::CompilerInvocation::CreateFromArgsImpl: " << targetAddr << " -> " << (void *)hooked_CreateFromArgsImplFunc << '\n';

    original_ptr = nullptr;
    DobbyHook(targetAddr, (void *)hooked_CreateFromArgsImplFunc, &original_ptr);
    original_CreateFromArgsImplFunc = reinterpret_cast<CreateFromArgsImplFunc>(original_ptr);
}
