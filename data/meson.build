project_source_files += [
    files('keplerian_elements.c'),
]

# Verify bsc5 downloaded correctly
bsc5_bin_path = 'bsc5'
bsc5_bin_hash = 'e471d02eaf4eecb61c12f879a1cb6432ba9d7b68a9a8c5654a1eb42a0c8cc340'
bsc5_ascii_path = 'ybsc5'
bsc5_ascii_hash = '69797549cc1605aad7ff94e9325e29a1661f2a253917faaa056d9bf20b809afd'

use_bsc5_ascii = false
bsc5_generated_path = 'bsc5_generated'

if fs.exists(bsc5_bin_path)
    expected_hash = bsc5_bin_hash
    bsc5_path = bsc5_bin_path
elif fs.exists(bsc5_ascii_path)
    use_bsc5_ascii = true
    expected_hash = bsc5_ascii_hash
    bsc5_path = bsc5_ascii_path
else
    error('bsc5 data not provided in binary or ascii form.')
endif

computed_hash = fs.hash(bsc5_path, 'sha256')
if computed_hash != expected_hash
  error(bsc5_path + ' SHA-256 hash mismatch! Build halted.' +
        '\nExpected:\t' + expected_hash +
        '\nGot:\t\t' + computed_hash)
else
  message(bsc5_path + ' SHA-256 hash verified successfully: ' + computed_hash)
endif

# Convert ascii to binary format
# This is not a one-to-one conversion (some floats are off by a factor of 10e-18)
# but should be very close to the actual binary data. We can verify this by running
# the standard test suite.
if use_bsc5_ascii

    python_exe = find_program('python3', 'python')
    script = '../scripts/bsc5_ascii_to_bin.py'

    r = run_command(
        python_exe, script, '--input', bsc5_ascii_path, '--output', bsc5_generated_path, check: true
    )

    # Check the return code and handle errors
    if r.returncode() != 0
        error('Error: Failed to convert ASCII to binary.\n' + r.stdout() + r.stderr())
    else
        message('bsc5 conversion to binary successful')
    endif

    # Update path to generated binary
    bsc5_path = bsc5_generated_path

endif
