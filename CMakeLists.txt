cmake_minimum_required(VERSION 3.15)
project(Preval-C)

set(C_STANDARD 17)

add_executable(Preval-C main.c memtracker.c operator.c parser.c tokeniser.c type.c compiler.c sb.c)