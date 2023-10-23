#
# Copyright 2023 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Calculate hash values for license_check_tb
"""

import hashlib

serial = [0x01, 0x23, 0x45, 0x67, 0x8A, 0xBC, 0xDE, 0xF0, 0x0C, 0x0D, 0xE0, 0xBB]
pkey = [0x00] * 37 + [0x05, 0xEC]
feature0 = [0x00, 0x00, 0xC0, 0xDE]
feature1 = [0x00, 0x00, 0xF0, 0x0D]

def main():
    """
    Calc and print hashes
    """
    sha0 = hashlib.sha256()
    sha0.update(bytes(serial + pkey + feature0))
    hash0 = sha0.hexdigest()
    sha1 = hashlib.sha256()
    sha1.update(bytes(serial + pkey + feature1))
    hash1 = sha1.hexdigest()

    print(f"// Feature 0 hash: 256'h{hash0}")
    hash0_words = [f"hash0_{int(i/8)} = 32'h" + hash0[i:i+8] for i in range(0, 64, 8)]
    print("// Individual words:")
    print(";\n localparam ".join(hash0_words))
    print(f"// Feature 1 hash: 256'h{hash1}")
    hash1_words = [f"hash1_{int(i/8)} = 32'h" + hash1[i:i+8] for i in range(0, 64, 8)]
    print("// Individual words:")
    print(";\n localparam ".join(hash1_words))

if __name__ == "__main__":
    main()
