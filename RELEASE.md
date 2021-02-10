Release process
===============

The `wimboot` release process is mildly convoluted due to the
requirement for UEFI Secure Boot signing.

Release binaries are committed to the source tree since they are
irreplaceable: the signed versions cannot ever be recreated if lost,
and the as-submitted unsigned versions are required to verify that no
code was altered by the external signing process.

Prerelease (unsigned binaries)
------------------------------

1. Ensure that all checks are passing on the current code, and that
   there are zero defects reported via [Coverity
   Scan](https://scan.coverity.com/projects/ipxe-wimboot).

1. Edit [`src/Makefile`](src/Makefile) to update `VERSION`.

2. Edit [`CHANGELOG.md`](CHANGELOG.md) to create a section and link
   for the new release.

3. Rebuild with the new version number:
   ```
   make -C src clean all
   ```

4. Commit these changes with a message such as:
   ```
   [release] Release version 2.7.0
   ```

5. Tag the commit, e.g.:
   ```
   git tag v2.7.0
   ```

6. Push the tag (and only the tag), e.g.
   ```
   git push origin v2.7.0
   ```
   This will automatically create a prerelease including the unsigned
   binaries as committed to the source tree.

7. When the tag checks have completed successfully, push as normal:
   ```
   git push
   ```

8. Submit `src/wimboot.cab` for UEFI Secure Boot signing using
   whatever process is current at the time.

Full release (signed binaries)
------------------------------

1. Wait for Microsoft to sign the UEFI Secure Boot submission.

2. Create a signed binary branch based on the release tag, e.g.:
   ```
   git checkout -b v2.7.0-signed v2.7.0
   ```

3. Download the `.zip` file containing the signed binaries.

4. Unzip the signed binaries using:
   ```
   unzip -d src -o -DD *.zip
   ```

5. Commit the signed binaries with a message such as:
   ```
   [release] Release version 2.7.0-signed
   ```

6. Verify that the binaries have not been altered by the signing
   process:
   ```
   make -C src
   ```

7. Check that the verification process has not modified any files:
   ```
   git status
   ```

8. Push the signed binary branch, e.g.:
   ```
   git push origin v2.7.0-signed
   ```
   This will automatically replace the unsigned binaries and convert
   the prerelease to a full release.
