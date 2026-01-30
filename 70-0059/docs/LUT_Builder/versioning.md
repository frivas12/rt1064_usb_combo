# LUT Versioning
Individual LUT files (i.e. the config and device LUTs) do not have a concept of versioning.
This presents a maintenance problem for LUTs.
To resolve this issue, the version file (EFS ID `0x62`) was introduced to record versioning information for LUT files. 
## Layout
The LUT version file is an array of 32-byte packs (minimum 3 packs).
The index in the array corresponds to the file id of the EFS type.
### Version File Index
The third item in the pack has the index of the layout file itself.
This is treated specially to contain header information.

| **Name**             | Length (bytes) | Offset (bytes) |
| :------------------- | -------------: | -------------: |
| **Inclusion Bitmap** | 4              | 0              |
| **Major Version**    | 1              | 4              |
| **Minor Version**    | 1              | 5              |
| **Path Version**     | 2              | 6              |
| **Reserved**         | 24             | 8              | 

The inclusion bitmap is a 32-bit (little endian) bitmap where a `1` indicates the EFS file with type `LUT` and the id corresponding  to the bit index is in the LUT version.
The version file's bit must always be set.
For example, if EFS files `0x60`, `0x61`, and `0x65` were in the LUT version, this bit field would be `0x27`.
The version file is guaranteed to have the byte pack of all files with a `1` in the inclusion bitmap.
Thus, the version file ranges between 96 bytes and 1024 bytes, the size equal to `32 * max_file_id`.

The major, minor, and patch versions can be used to identify the LUT package from another LUT package.
The patch version is stored in little endian.
### Non-Version File Index
For byte packs with indices other than the version index, the 32 bytes represent the SHA-256 hash of the contents of the LUT file.
If the file cannot fill the contents of a SHA-256 block (512 bits per block), the bits will be filled with `0`.

For the LUT package to be valid, the SHA-256 hash of the embedded file must be equal to that of the hash stored in the file index.
This check is **not** done at runtime.

## Creating Versions
To create a version file, run the `version-compiler.py` file in the ["look-up-tables"](../../proj/look-up-tables/) project and input a CSV file with a version number (major.minor.patch).

The first column in the CSV file is the EFS index.
If this value is not within the range [0,31], the compiler will raise an error.
If this value is set to 2 (the version file identifier), the compiler will raise an error.

The second column in the CSV file is the path to the LUT file.
For relative paths, this is calculated from the working directory of the executing python script.
If this file cannot be loaded, the compiler raises an error.

Newlines can be used to add additional files.

