import subprocess
import os
import time
import psutil
import shutil
import resource

disk_limit_mb_all = [512] * 62 + [1024] * 10 + [384] * 30 + [512] * 10

# 全局变量用于传递内存和时间限制
global_memory_limit_mb = 0
global_time_limit_ms = 0

def compare_files(file1, file2):
    def read_lines(filename):
        with open(filename, 'r') as file:
            # 使用 rstrip 去除每行末尾的空白字符，并将结果存储在一个列表中
            lines = [line.rstrip() for line in file]
            # 去除列表末尾的空字符串（可能由文件末尾多余的空行或回车造成）
            while lines and lines[-1] == '':
                lines.pop()
            return lines

    lines1 = read_lines(file1)
    lines2 = read_lines(file2)

    if len(lines1) != len(lines2):
        print(f"Files {file1} and {file2} have different number of lines.")
        return False

    for line_no, (line1, line2) in enumerate(zip(lines1, lines2), start=1):
        if line1 != line2:
            print(f"Difference at line {line_no}:")
            print(f"Your answer: {line1}")
            print(f"Standard answer: {line2}")
            return False

    return True


def set_limits():
    memory_limit_bytes = global_memory_limit_mb * 1024 * 1024
    cpu_time_limit_seconds = global_time_limit_ms // 1000

    # 设置最大可用内存
    resource.setrlimit(resource.RLIMIT_AS, (memory_limit_bytes, memory_limit_bytes))

    # 设置最大打开文件数
    max_files = 50
    resource.setrlimit(resource.RLIMIT_NOFILE, (max_files, max_files))

    # 设置最大CPU时间
    resource.setrlimit(resource.RLIMIT_CPU, (cpu_time_limit_seconds, cpu_time_limit_seconds))

def get_directory_size(directory):
    total_size = 0
    with os.scandir(directory) as it:
        for entry in it:
            if entry.is_file():
                total_size += entry.stat().st_size
            elif entry.is_dir():
                total_size += get_directory_size(entry.path)
    return total_size

def execute_program(program_name, input_file, output_file, expected_output, time_limit_ms, memory_limit_mb, disk_limit_mb):
    global global_memory_limit_mb
    global global_time_limit_ms

    global_memory_limit_mb = memory_limit_mb
    global_time_limit_ms = time_limit_ms

    start_time = time.perf_counter()

    process = subprocess.Popen(
        program_name + " < " + input_file + " > " + output_file,
        preexec_fn=set_limits,
        cwd="temp/",
        stderr=subprocess.PIPE,
        shell=True
    )

    max_memory_usage = 0
    while process.poll() is None:
        try:
            current_memory_usage = psutil.Process(process.pid).memory_info().rss / (1024 * 1024)  # 转换为MB
            if current_memory_usage > max_memory_usage:
                max_memory_usage = current_memory_usage
        except psutil.NoSuchProcess:
            break


    _, stderr = process.communicate()

    end_time = time.perf_counter()
    execution_time = (end_time - start_time) * 1000.0  # 将时间转换为毫秒

    if process.returncode == -9:  # 检查是否因时间限制被终止
        return False, "Time limit exceeded", max_memory_usage, ""

    if process.returncode != 0:
        return False, f"Process failed with return code {process.returncode} : {stderr.decode()}", max_memory_usage, ""

    if not compare_files(temp_directory + output_file, expected_output):
        # subprocess.Popen(
        #     "diff " + temp_directory + output_file + " " + expected_output,
        #     shell=True
        # )
        return False, "Output does not match the expected output", max_memory_usage, ""

    subprocess.Popen("rm " + temp_directory + output_file, shell=True)
    if get_directory_size(temp_directory) > disk_limit_mb * 1024 * 1024:
        return (False,
                f"Disk Memory Limit Exceeded: {get_directory_size(temp_directory / 1024 / 1024)}/{disk_limit_mb} MB",
                max_memory_usage)

    return True, execution_time, max_memory_usage, get_directory_size(temp_directory) / 1024 / 1024


def clear_directory(directory):
    for file in os.listdir(directory):
        file_path = os.path.join(directory, file)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print(f'Failed to delete {file_path}. Reason: {e}')


def main(test_ranges, program_name, input_prefix, output_prefix, expected_output_prefix, temp_directory, time_limits_ms,
         memory_limits_mb):
    global disk_limit_mb_all
    for test_group in test_ranges:
        clear_directory(temp_directory)
        for i in range(test_group[0], test_group[1] + 1):
            input_file = f"{input_prefix}{i}.in"
            output_file = f"{output_prefix}{i}.out"
            expected_output = f"{expected_output_prefix}{i}.out"

            time_limit_ms = time_limits_ms[i - 1]  # 获取当前测试点的时间限制（毫秒）
            memory_limit_mb = memory_limits_mb[i - 1]  # 获取当前测试点的内存限制（MB）
            disk_limit_mb = disk_limit_mb_all[i - 1]

            result, result_info, memory_usage, disk_usage = execute_program(program_name, input_file, output_file, expected_output,
                                                                time_limit_ms, memory_limit_mb, disk_limit_mb)

            if not result:
                print(f"Test {i} in group {test_group} failed: {result_info}")
                return

            print(
                f"Test {i} in group {test_group} passed: Time taken: {result_info:.2f}/{time_limit_ms} ms, Memory usage: {memory_usage: .2f}/{memory_limit_mb} MB, Disk usage: {disk_usage: .2f}/{disk_limit_mb} MB")


if __name__ == "__main__":
    test_ranges = [
        (1, 1), (2, 2), (3, 7), (8, 12), (13, 22), (23, 32), (33, 42), (43, 52),
        (53, 62), (63, 72), (73, 82), (83, 92), (93, 102)
    ]  # 定义你的测试范围
    program_name = "./../cmake-build-debug/code"
    input_prefix = "../testcases/"
    output_prefix = ""
    expected_output_prefix = "testcases/"
    temp_directory = "temp/"

    # 定义你的时间限制数组（每个测试点的时间限制，单位：毫秒）
    time_limits_ms = [6000] * 32 + [20000] * 10 + [80000] * 30 + [36000] * 30 + [48000] * 10
    # 定义你的内存限制数组（每个测试点的内存限制，单位：MB）
    memory_limits_mb = [25] * 72 + [16] * 30

    main(test_ranges, program_name, input_prefix, output_prefix, expected_output_prefix, temp_directory, time_limits_ms,
         memory_limits_mb)
