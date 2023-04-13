# Flay
Flay is a control-plane dead code elimination tool for P4 programs. Flay takes a P4 program and control-plane configuration and removes all the code that can not be executed with this particular configuration. Flay also removes code that is dead across programmable blocks. For example, code that is only executed if a header is valid, but that header is never set valid in the parser block.

## Building Flay
Flay is a (P4Tools)[https://github.com/p4lang/p4c/tree/main/backends/p4tools] module, which itself is part of (P4C)[https://github.com/p4lang/p4c]. To be able to use Flay, you first need to build P4C. Instructions can be found (here)[https://github.com/p4lang/p4c#installing-p4c-from-source].

After building P4C, you need link Flay as a P4Tools module. The (CI scripts)[https://github.com/fruffy/flay/blob/master/.github/workflows/ci-build.yml] describe a similar workflow.
You will need to symlink Flay into the modules directory, rerun CMake in your P4C build directory, then rebuild.
```
ln -sf flay p4c/backends/p4tools/modules/
cd p4c/build
cmake ..
make
```
