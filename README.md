## XcodeClangInjectPlugin

#### 编译
```bash
mkdir llvm_work
cd llvm_work
git clone --depth 1 -b 'llvmorg-16.0.0' git@github.com:llvm/llvm-project.git
git clone https://github.com/0x1306a94/XcodeClangInjectPlugin
mkdir build_ninja
cd build_ninja
cmake -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install -DLLVM_ABI_BREAKING_CHECKS=FORCE_OFF -DLLVM_ENABLE_ZSTD=OFF -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DLLVM_ABI_BREAKING_CHECKS="FORCE_ON" -DLLVM_EXTERNAL_PROJECTS=XcodeClangInjectPlugin -DLLVM_EXTERNAL_XCODECLANGINJECTPLUGIN_SOURCE_DIR=../XcodeClangInjectPlugin  ../llvm-project/llvm
ninja install_CheckerCodeStylePluginLoader
```

#### 生成Xcode工程
```bash
cmake -G Xcode -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install -DLLVM_ABI_BREAKING_CHECKS=FORCE_OFF -DLLVM_ENABLE_ZSTD=OFF -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DLLVM_ABI_BREAKING_CHECKS="FORCE_ON" -DLLVM_EXTERNAL_PROJECTS=XcodeClangInjectPlugin -DLLVM_EXTERNAL_XCODECLANGINJECTPLUGIN_SOURCE_DIR=../XcodeClangInjectPlugin  ../llvm-project/llvm
```