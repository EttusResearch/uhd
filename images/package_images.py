#!/usr/bin/env python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Package image files into image archive packages

Provides functions for packaging image files into image packages. Generate the intermediate files
(like hash files), and create image archives from sets.
"""
from __future__ import print_function
import argparse
import copy
import glob
import hashlib
import itertools
import os
import re
import sys
import tempfile
import zipfile
from image_package_mapping import PACKAGE_MAPPING


def parse_args():
    """Setup argument parser and parse"""
    description = """UHD Image Packaging

    Packages the contents of the current directory into archives within a directory structure that
    matches the Ettus fileserver. It also produces files containing the MD5 checksums of all image
    files, as well as a file containing the SHA256 checksums of all archive files created.

    The script will also modify a manifest file with the information from the generated image
    packages. That is, the repositories, Git hashes, and SHA256 checksums listed in the manifest
    will be updated.

    The script will run without any commandline arguments provided. However, some useful (crucial,
    even) information will be lacking. The suggested usage is to invoke the following command from
    the directory containing the image files

    `python package_images.py --manifest /path/to/manifest --githash <REPO>-<GITHASH>`

    where REPO is the repository used to create the images  (ie 'fpga'), and GITHASH is the Git
    hash of that repository used to create the images. When in doubt, please check with previous
    image package listed in the manifest.
    """
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description)
    parser.add_argument('--md5', action="store_true", default=False,
                        help="Generate MD5 files")
    parser.add_argument('--sha256', action="store_true", default=False,
                        help="Generate SHA256 files")
    parser.add_argument('-f', '--files', type=str, default="",
                        help="Comma separate list of files")
    parser.add_argument('-o', '--output', type=str, default="",
                        help="Output file to put the hashes in")
    parser.add_argument('-m', '--manifest', type=str, default="",
                        help="Update the manifest file at this path with the new SHAs")
    parser.add_argument('-t', '--targets', type=str, default="",
                        help="RegEx to select image sets from the manifest file.")
    parser.add_argument('-g', '--githash', type=str, default="",
                        help="Git hash directory name (eg. fpga-abc1234)")
    return parser.parse_args()


def gen_filelist(includes, excludes=None):
    """
    Generates a list of files, first generating
    :param includes: [list of] expression[s] to include
    :param excludes: [list of] expression[s] to exclude
    :return: flat list of filenames
    """
    if isinstance(includes, str):
        included = glob.glob(includes)
    else:
        included = list(itertools.chain(*[glob.iglob(filename) for filename in includes]))

    if excludes is None:
        excluded = []
    elif isinstance(excludes, str):
        excluded = glob.glob(excludes)
    else:
        excluded = list(itertools.chain(*[glob.iglob(filename) for filename in excludes]))
    # Remove the excluded files from the include list
    for filename in excluded:
        if filename in included:
            included.remove(filename)
    return included


def gen_md5(files_list, hash_filename=""):
    """Generate the .md5 files for all input files"""
    hashes = {}
    for filename in files_list:
        # Read and hash the input file
        with open(filename, 'rb') as img_file:
            md5_sum = hashlib.md5()
            md5_sum.update(img_file.read())
        # Write the hash to a *.md5 file
        with open(filename + '.md5', 'w') as md5_file:
            md5_hex = md5_sum.hexdigest()
            newline = "{md5_hex}  {filename}\n".format(filename=filename, md5_hex=md5_hex)
            md5_file.write(newline)
            # Also store it to write to a file of all the hashes
            hashes[filename] = md5_hex

    # Write the MD5 hashes to file
    with open(hash_filename, 'a') as hash_file:
        for filename, md5_hex in hashes.items():
            newline = "{md5_hex}  {filename}\n".format(filename=filename, md5_hex=md5_hex)
            hash_file.write(newline)


def gen_sha256(files_list, hash_filename=None, manifest_fn="", repo_and_hash=""):
    """Generate the SHA256 files for all input file"""
    # Input checking
    if hash_filename is None:
        hash_filename = "hashes.txt"
    print("Generating SHA256 sums for:\n{}".format(
        "\n".join("--{}".format(sha_fn) for sha_fn in files_list)))

    # Make a dictionary to store the new SHA256 sums
    sha256_dict = {}
    with open(hash_filename, 'a') as hash_file:
        for filename in files_list:
            with open(filename, 'rb') as img_file:
                sha256_sum = hashlib.sha256()
                sha256_sum.update(img_file.read())
                sha256_str = sha256_sum.hexdigest()
                newline = "{sha_hex}  {filename}\n".format(filename=filename,
                                                           sha_hex=sha256_str)
                hash_file.write(newline)
                # Add the sha256 to the dictionary
                basename = os.path.basename(filename)
                sha256_dict[basename] = sha256_str

    # If there's a manifest file to edit, put the new information in
    if os.path.isfile(manifest_fn):
        edit_manifest(manifest_fn, repo_and_hash, sha256_dict)


def gen_zip(zip_filename, files_list):
    """Generate the zip file for a set of images"""
    try:
        with zipfile.ZipFile(zip_filename, 'w', zipfile.ZIP_DEFLATED) as zip_file:
            for filename in files_list:
                zip_file.write(filename)
        return True
    except Exception as ex:
        print("Caught exception in gen_zip: {}".format(ex))
        return False


def do_gen_package(pkg_target, install_dir="", repo_and_hash=""):
    """Generate the entire N3XX image package, from the start to the end"""
    print("---Generating package for {}---".format(pkg_target))
    filelist = PACKAGE_MAPPING[pkg_target]['files']
    print("Required files:\n{}".format(
        "\n".join("--{}".format(img_fn) for img_fn in filelist)))
    md5_files = gen_filelist(includes=filelist, excludes=["*.rpt", "*.md5"])
    print("Files to md5sum:\n{}".format(
        "\n".join("--{}".format(md5_fn) for md5_fn in md5_files)))
    gen_md5(md5_files, "md5_hashes.txt")

    # Determine the current Git hash (w/o the repository)
    githash_l = re.findall(r"[\d\w]+-([\d\w]{7,8})", repo_and_hash)
    githash = githash_l[0] if githash_l else ""

    zip_files = gen_filelist(includes=filelist)
    zip_filename = os.path.join(install_dir, PACKAGE_MAPPING[pkg_target]['package_name'])\
        .format(githash)
    print("Files to zip:\n{}".format(
        "\n".join("--{}".format(zip_fn) for zip_fn in zip_files)))
    if not gen_zip(zip_filename, zip_files):
        zip_filename = ""
    return zip_filename


def gen_package(pkg_targets=(), repo_and_hash="", manifest_fn=""):
    """Generate the entire image package, and place it in the proper directory structure"""
    # Make the cache/ directory if necessary
    cache_path = os.path.join(os.getcwd(), "cache")
    if not os.path.isdir(cache_path):
        os.mkdir(cache_path)

    sha_filenames = []
    for pkg_target in pkg_targets:
        if pkg_target in PACKAGE_MAPPING:
            # Make the type directory
            pkg_type = PACKAGE_MAPPING[pkg_target]["type"]
            type_path = os.path.join(cache_path, pkg_type)
            if not os.path.isdir(type_path):
                os.mkdir(type_path)
            # Make the 'repository-hash' directory
            if not repo_and_hash:
                repo_and_hash = "repo-githash"
            git_path = os.path.join(type_path, repo_and_hash)
            if not os.path.isdir(git_path):
                os.mkdir(git_path)

            # Generate the package and add the the zip filename to the SHA list
            sha_filenames.append(do_gen_package(pkg_target,
                                                install_dir=git_path,
                                                repo_and_hash=repo_and_hash))
        else:
            print("Error: Specify a supported type from {}".format(
                list(PACKAGE_MAPPING.keys())))
    sha_filenames[:] = [sha_fn for sha_fn in sha_filenames if os.path.exists(sha_fn)]
    gen_sha256(sha_filenames, hash_filename="hashes.txt",
               manifest_fn=manifest_fn, repo_and_hash=repo_and_hash)
    # Return the zipfiles we've created
    return sha_filenames


def list_differences(list1, list2):
    """Returns two lists containing the unique elements of each input list"""
    outlist1 = []
    outlist2 = []
    outlist1[:] = [elem for elem in list1 if elem not in list2]
    outlist2[:] = [elem for elem in list2 if elem not in list1]
    return outlist1, outlist2


def get_target_name(zip_filename):
    """Return the package target that created the given zip_filename"""
    for target, target_info in PACKAGE_MAPPING.items():
        # First we need to strip the Git hash out of the filename
        githash = re.findall(r"-g([\d\w]{7,8})", zip_filename)[0]
        stripped_filename = os.path.basename(zip_filename.replace(githash, "{}"))
        if stripped_filename == target_info.get("package_name", ""):
            return target
    # If it doesn't match any targets
    return ""


def verify_package(zip_filename):
    """Verify the contents of the image package match the expected list of files"""
    # First, determine which target this was built for
    pkg_target = get_target_name(zip_filename)
    if not pkg_target:
        print("Error: Could not determine package from filename",
              file=sys.stderr)
        return False

    expected_filelist = PACKAGE_MAPPING[pkg_target]['files']
    with zipfile.ZipFile(zip_filename, 'r') as zip_file:
        actual_filelist = zip_file.namelist()

    missing, extra = list_differences(expected_filelist, actual_filelist)
    if missing or extra:
        print("Error: image package does not include expected files ({})".format(pkg_target),
              file=sys.stderr)
        if missing:
            print("Missing files: {}".format(missing), file=sys.stderr)
        if extra:
            print("Extra files: {}".format(extra), file=sys.stderr)
        return False
    return True


def edit_manifest_line(line, new_repo_and_hash, new_hashes_dict):
    """Edit the line in the manifest to (maybe) include the new repo, git hash, and SHA"""
    # Check each value in your dictionary of new hashes
    for filename, new_hash in new_hashes_dict.items():
        # If the filename with a new hash shows up in the line
        # Note: the filename has a Git hash in it, so we need to peel that off first
        full_filename_matches = re.findall(r"([\d\w]+)-g([\da-fA-F]{7,8})", filename)
        if full_filename_matches:
            # We don't really need to store the Git hash in the found filename
            stripped_filename, _ = full_filename_matches[0]
        else:
            return line

        if stripped_filename in line:
            # Replace the repo and git hash
            old_repo_and_hash_matches = re.findall(r"([\w]+)-([\da-fA-F]{7,8})", line)
            if old_repo_and_hash_matches:
                # If we did find a repo and Git hash on this line, replace them
                old_repo, old_githash = old_repo_and_hash_matches[0]
                old_repo_and_hash = "{}-{}".format(old_repo, old_githash)
                # We need to replace all instances <REPO>-<GITHASH> in this line
                line = line.replace(old_repo_and_hash, new_repo_and_hash)
                # We also need to replace -g<GITHASH> in the filename
                # Find the new repo and githash
                _, new_githash = re.findall(r"([\w]+)-([\da-fA-F]{7,8})", new_repo_and_hash)[0]
                line = line.replace(old_githash, new_githash)

            # Replace the SHA256
            sha = re.findall(r"[\da-fA-F]{64}", line)
            if sha:
                sha = sha[0]
                line = line.replace(sha, new_hash)

            if not old_repo_and_hash_matches or not sha:
                print("Error: repo, hash or SHA missing in line with new file")
                print("Line: {}", format(line))
            # If we found and replaced info, return the edited line
            return line
    # If we never edit the line, just return it
    return line


def edit_manifest(manifest_fn, new_repo_and_hash, new_hash_dict):
    """Edit the provided manifest file to update the githash and SHA256"""
    with tempfile.NamedTemporaryFile(mode='w', dir='.', delete=False) as tmp_manifest, \
            open(manifest_fn, 'r') as old_manifest:
        print("Trying to edit manifest with new repo and Git hash {}".format(new_repo_and_hash))
        # Check each line in the manifest file
        for line in old_manifest:
            # If needed, put the new info in the line
            line = edit_manifest_line(line, new_repo_and_hash, new_hash_dict)
            # Always write the line back
            tmp_manifest.write(line)
    # Replace the manifest file with our temporary file that we created
    os.rename(tmp_manifest.name, manifest_fn)


def determine_targets():
    """
    Determine which image packages can be created by the files in the current directory
    :return: list of valid targets
    """
    found_targets = []
    for target, target_info in PACKAGE_MAPPING.items():
        # Grab the list of files required, but remove any files that we're going to build here,
        # like the hash files
        required_files = copy.deepcopy(target_info['files'])
        required_files[:] = [filename for filename in required_files if '.md5' not in filename]

        check_required_files = [os.path.exists(img_file) for img_file in required_files]
        if all(check_required_files):
            found_targets.append(target)
        elif any(check_required_files):
            print("Not all package contents present for {}".format(target),
                  file=sys.stderr)
    return found_targets


def main():
    """Generate image packages using commandline arguments"""
    args = parse_args()
    if args.md5 or args.sha256 or args.files or args.output:
        print("Unsupported argument: only --pkg_targets is currently supported.")
    # Check the provided Git hash
    if not args.githash:
        print("Please provide --githash `<REPO>-<GITHASH>'")
        return False
    elif not re.findall(r"[\d\w]+-[\d\w]{7,8}", args.githash):
        print("--githash does not match expected form. Should be `<REPO>-<GITHASH>'")
        return False

    if args.targets:
        pkg_targets = [ss.strip() for ss in args.targets.split(',')]
    else:
        pkg_targets = determine_targets()
        print("Targets to package:\n{}".format(
            "\n".join("--{}".format(pkg) for pkg in pkg_targets)))

    zip_filenames = gen_package(pkg_targets=pkg_targets,
                                repo_and_hash=args.githash,
                                manifest_fn=args.manifest)
    check_zips = [verify_package(zip_filename) for zip_filename in zip_filenames]
    return all(check_zips)


if __name__ == "__main__":
    sys.exit(not main())
