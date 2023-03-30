cmake_minimum_required(VERSION 3.0)

project(shell)

aux_source_directory(. DIR_SRC)

add_executable(shell shell.c ${DIR_SRC})