#!/usr/bin/python3
import os
import argparse
from natsort import natsorted
from functional import seq
import subprocess
from termcolor import colored, cprint
import json
from multiprocessing import Pool, Lock

TESTDATA_DIR = os.path.abspath(os.path.dirname(__file__))
PROJECT_DIR = os.path.dirname(TESTDATA_DIR)

RESULT_DIR = os.path.join(PROJECT_DIR, 'tests', 'result')  # temp dir

BUILD_DIR = os.path.join(PROJECT_DIR, 'build')
PARSER_EXECUTABLE = os.path.join(BUILD_DIR, 'parser')
SEMANTIC_EXECUTABLE = os.path.join(BUILD_DIR, 'semantic')
IR_EXECUTABLE = os.path.join(BUILD_DIR, 'ir-optimizer')
CGEN_EXECUTABLE = os.path.join(BUILD_DIR, 'cgen')

print_lock = Lock()


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
        student_ast_has_error = len(student_ast['errors']['errors']) > 0
        reference_ast_has_error = len(reference_ast['errors']['errors']) > 0
        if student_ast_has_error and reference_ast_has_error:
            return True
        if student_ast_has_error or reference_ast_has_error:
            if verbose:
                cprint('Expected AST has error, but your AST does not have error',
                       'yellow')
                print('Your AST', colored(student_ast['errors'], 'dark_grey'))
                print('Expected AST',
                      colored(reference_ast['errors'], 'dark_grey'))
            return False
    except Exception as e:
        return False
    return compare_ast_node(student_ast, reference_ast, verbose=verbose)


def check_testcase(directory: str, testcase: str, pa: int) -> int:
    assert 1 <= pa <= 4
    if pa == 1:
        assert os.path.exists(PARSER_EXECUTABLE)
        assert os.path.isfile(PARSER_EXECUTABLE)
    elif pa == 2:
        assert os.path.exists(SEMANTIC_EXECUTABLE)
        assert os.path.isfile(SEMANTIC_EXECUTABLE)
    elif pa == 3:
        assert os.path.exists(IR_EXECUTABLE)
        assert os.path.isfile(IR_EXECUTABLE)
    elif pa == 4:
        assert os.path.exists(CGEN_EXECUTABLE)
        assert os.path.isfile(CGEN_EXECUTABLE)

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
        3: [
            IR_EXECUTABLE,
            '-o', os.path.join(RESULT_DIR, testcase),
            '-run', os.path.join(directory, f'{testcase}.py')
        ],
        4: [CGEN_EXECUTABLE,
            '-o', os.path.join(RESULT_DIR, testcase),
            '-run', os.path.join(directory, f'{testcase}.py')]
    }[pa]

    try:
        p = subprocess.run(command, timeout=3, capture_output=True)
        student_output = p.stdout.decode()
    except subprocess.TimeoutExpired:
        with print_lock:
            cprint(f'[{testcase}] Timeout!', 'red')
        return 0
    except Exception as e:
        with print_lock:
            cprint(f'[{testcase}] [Runtime error: {e}]', 'red')
        return 0
    if p.returncode != 0:
        with print_lock:
            cprint(
                f'[{testcase}] [Return value != 0, stderr: {p.stderr.decode()}]', 'red')
        return 0

    if pa == 1 or pa == 2:
        # Load AST
        reference_ast = json.loads(reference_output)
        try:
            student_ast = json.loads(student_output)
        except Exception as e:
            with print_lock:
                cprint(f'[{testcase}] [Parse JSON error: {e}]', 'red')
            return 0

        # Compare AST
        if compare_ast(student_ast, reference_ast):
            with print_lock:
                cprint(f'[{testcase}] Passed', 'green')
            return 1
        else:
            with print_lock:
                print(f'[{testcase}]')
                compare_ast(student_ast, reference_ast, verbose=True)
                cprint(f'Failed!', 'red')
            return 0
    elif pa == 3 or pa == 4:
        # Compare program output
        if student_output.strip() == reference_output.strip():
            with print_lock:
                cprint(f'[{testcase}] Passed', 'green')
            return 1
        else:
            with print_lock:
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
    with Pool() as p:
        results = p.starmap(check_testcase, [
            (directory, testcase, pa)
            for testcase in testcases
        ])
    ret = sum(results), len(results)
    print(f'(Passed, Total) = {ret}')
    return ret


def check_pa(pa: int):
    pa_directory = os.path.join(TESTDATA_DIR, f'pa{pa}')
    subdirectories = ['sample']
    subdirectories.append('sp23')
    results = [
        check_testcases_in_directory(os.path.join(pa_directory, sub), pa)
        for sub in subdirectories
    ]


if __name__ == '__main__':
    print(f'[testdata: {TESTDATA_DIR}]')
    print(f'[executable: {BUILD_DIR}]')
    if os.path.exists(RESULT_DIR):
        import shutil
        shutil.rmtree(RESULT_DIR)
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
