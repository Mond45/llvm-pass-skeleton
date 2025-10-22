clang -Xclang -disable-O0-optnone -S -emit-llvm -o - -fpass-plugin=`echo build/skeleton/SkeletonPass.*` something.c
