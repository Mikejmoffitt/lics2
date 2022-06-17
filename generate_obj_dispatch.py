#!/usr/bin/python3
# Dirty tool to generate the const obj dispatch array.
# First argument is source object directory (containing headers).
# Second argument is the output filename.

import os
import sys

header_list = []
decl_list = []

def maybe_generate_entry(in_path, filename):
	if ".swp" in filename: return
	if ".h" not in filename: return
	obj_name_base = filename.split(".")[0].strip()

	load_func = "o_load_" + obj_name_base
	unload_func = "o_unload_" + obj_name_base

	with open(in_path + "/" + filename, "r") as src_f:
		source_str = src_f.read();
		if load_func not in source_str:
			load_func = "NULL"
		if unload_func not in source_str:
			unload_func = "NULL"

	line = "\t[OBJ_" + obj_name_base.upper() + "] = "
	line = line + "{" + load_func + ", " + unload_func + "},\n"
	header_list.append("#include \"obj/" + obj_name_base + ".h\"\n")
	decl_list.append(line)

def generate(table_name, in_path, out_fname):
	with open(out_fname, "w", encoding="utf-8") as f:
		for filename in os.listdir(in_path):
			maybe_generate_entry(in_path, filename)
		for header_str in header_list:
			f.write(header_str)
		f.write("static const SetupFuncs " + table_name + "[] =\n")
		f.write("{\n")
		for decl_str in decl_list:
			f.write(decl_str)
		f.write("};\n")

if __name__ == "__main__":
	if len(sys.argv) < 4:
		print("Usage: ", sys.argv[0], " table_name, obj_dir, out_fname\n")
	else:
		generate(sys.argv[1], sys.argv[2], sys.argv[3])

