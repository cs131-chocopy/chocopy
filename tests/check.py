#!/usr/bin/python3
import os
import argparse
from natsort import natsorted
from functional import seq
import subprocess
from termcolor import colored, cprint
import json

TESTDATA_DIR = os.path.abspath(os.path.dirname(__file__))
PROJECT_DIR = os.path.dirname(TESTDATA_DIR)

RESULT_DIR = os.path.join(PROJECT_DIR, 'tests', 'result') # temp dir

BUILD_DIR = os.path.join(PROJECT_DIR, 'build')
PARSER_EXECUTABLE = os.path.join(BUILD_DIR, 'parser')
SEMANTIC_EXECUTABLE = os.path.join(BUILD_DIR, 'semantic')
CGEN_EXECUTABLE = os.path.join(BUILD_DIR, 'cgen')



def compare_ast_node(student_ast_node: dict, reference_ast_node: dict, verbose: bool = False) -> bool:
    assert isinstance(student_ast_node, dict)
    assert isinstance(reference_ast_node, dict)
    if student_ast_node.keys() != reference_ast_node.keys():
        if verbose:
            cprint(f'Expected {len(reference_ast_node.keys())} keys, but {len(student_ast_node.keys())} keys are found',
                   'yellow')
            print('Your AST keys',
                  colored(list(student_ast_node.keys()), 'yellow'))
            print('Expected AST keys',
                  colored(list(reference_ast_node.keys()), 'yellow'))
            print('Your AST', colored(student_ast_node, 'dark_grey'))
            print('Expected AST', colored(reference_ast_node, 'dark_grey'))
        return False
    for key in reference_ast_node.keys():
        student_value = student_ast_node[key]
        reference_value = reference_ast_node[key]

        if key == 'location':
            # Skip checking location now
            continue

        if isinstance(student_value, list):
            student_values = student_value
            reference_values = reference_value
        else:
            student_values = [student_value]
            reference_values = [reference_value]

        for [student_value, reference_value] in zip(student_values, reference_values):
            if type(student_value) != type(reference_value):
                if verbose:
                    cprint(f'Expected type {type(reference_value)} of `{key}`, but {type(student_value)} is found',
                           'yellow')
                    print('Your AST', colored(student_ast_node, 'dark_grey'))
                    print('Expected AST',
                          colored(reference_ast_node, 'dark_grey'))
                return False
            if isinstance(student_value, dict):
                if not compare_ast_node(student_value, reference_value, verbose=verbose):
                    return False
            else:
                if student_value != reference_value:
                    if verbose:
                        cprint(f'Expected value {reference_value} of `{key}`, but {student_value} is found',
                               'yellow')
                        print('Your AST',
                              colored(student_ast_node, 'dark_grey'))
                        print('Expected AST',
                              colored(reference_ast_node, 'dark_grey'))
                    return False
    return True


def compare_ast(student_ast: dict, reference_ast: dict, verbose: bool = False) -> bool:
    try:
        if len(student_ast['errors']['errors']) > 0 and len(reference_ast['errors']['errors']) > 0:
            return True
    except Exception as e:
        pass
    return compare_ast_node(student_ast, reference_ast, verbose=verbose)


def check_testcase(directory: str, testcase: str, pa: int) -> int:
    assert 1 <= pa <= 4
    assert os.path.exists(
        PARSER_EXECUTABLE) and os.path.isfile(PARSER_EXECUTABLE)

    reference_output_path: str = {
        1: os.path.join(directory, f'{testcase}.py.ast'),
        2: os.path.join(directory, f'{testcase}.py.out.typed'),
        3: os.path.join(directory, f'{testcase}.py.typed.ll.result'),
        4: os.path.join(directory, f'{testcase}.py.ast.typed.s.result'),
    }[pa]

    with open(reference_output_path, 'r') as f:
        reference_output = f.read()

    command = {
        1: [
            PARSER_EXECUTABLE,
            os.path.join(directory, f'{testcase}.py')
        ],
        2: [
            SEMANTIC_EXECUTABLE,
            os.path.join(directory, f'{testcase}.py')
        ],
        4: [CGEN_EXECUTABLE,
            '-o', os.path.join(RESULT_DIR, testcase),
            '-run', os.path.join(directory, f'{testcase}.py')]
    }[pa]

    try:
        p = subprocess.run(command, timeout=3, capture_output=True)
        student_output = p.stdout.decode()
    except subprocess.TimeoutExpired:
        cprint(f'[{testcase}] Timeout!', 'red')
        return 0
    except Exception as e:
        cprint(f'[{testcase}] [Error: {e}]', 'red')
        return 0

    if pa == 1 or pa == 2:
        # Load AST
        reference_ast = json.loads(reference_output)
        try:
            student_ast = json.loads(student_output)
        except Exception as e:
            cprint(f'[{testcase}] [Error: {e}]', 'red')
            return 0

        # Compare AST
        if compare_ast(student_ast, reference_ast):
            cprint(f'[{testcase}] Passed', 'green')
            return 1
        else:
            print(f'[{testcase}]')
            compare_ast(student_ast, reference_ast, verbose=True)
            cprint(f'Failed!', 'red')
            return 0
    elif pa == 3:
        raise NotImplementedError()
    elif pa == 4:
        # Compare program output
        if student_output == reference_output:
            cprint(f'[{testcase}] Passed', 'green')
            return 1
        else:
            cprint(f'[{testcase}] Failed!', 'red')
            return 0

    return 0


def check_testcases_in_directory(directory: str, pa: int) -> tuple[int, int]:
    if not (os.path.exists(directory) and os.path.isdir(directory)):
        print(f'[{directory} does not exist]')
        return 0, 0
    print(f'[checking {directory}]')
    testcases: list[str] = seq(natsorted(os.listdir(directory)))\
        .filter(lambda x: x.endswith('.py'))\
        .map(lambda x: x[:-3])\
        .to_list()
    results = [
        check_testcase(directory, testcase, pa)
        for testcase in testcases
    ]
    ret = sum(results), len(results)
    print(f'(Passed, Total) = {ret}')
    return ret


def check_pa(pa: int):
    pa_directory = os.path.join(TESTDATA_DIR, f'pa{pa}')
    subdirectories = ['sample']
    results = [
        check_testcases_in_directory(os.path.join(pa_directory, sub), pa)
        for sub in subdirectories
    ]


if __name__ == '__main__':
    print(f'[testdata: {TESTDATA_DIR}]')
    print(f'[executable: {BUILD_DIR}]')
    if not os.path.exists(RESULT_DIR):
        os.mkdir(RESULT_DIR)

    parser = argparse.ArgumentParser(description="Checker for ChocoPy")
    parser.add_argument("--pa", metavar="N", type=int, nargs="+")

    args = parser.parse_args()
    if args.pa is None:
        parser.print_help()
        os.sys.exit(1)

    pa = args.pa[0]
    if not 1 <= pa <= 4:
        print(f'There is no PA{pa}')
        os.sys.exit(1)

    print(f'[checking PA{pa}]')
    check_pa(pa)
