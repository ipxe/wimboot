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

2. Edit [`src/Makefile`](src/Makefile) to update `VERSION`, and to
   increment `SBAT_GENERATION` if needed (i.e. if the release fixes a
   new Secure Boot exploit).

3. Edit [`CHANGELOG.md`](CHANGELOG.md) to create a section and link
   for the new release.

4. Rebuild with the new version number:
   ```
   make -C src clean all
   ```

5. Commit these changes with a message such as:
   ```
   [release] Release version 2.7.0
   ```

6. Tag the commit, e.g.:
   ```
   git tag v2.7.0
   ```

7. Push the tag (and only the tag), e.g.
   ```
   git push origin v2.7.0
   ```
   This will automatically create a prerelease including the unsigned
   binaries as committed to the source tree.

8. When the tag checks have completed successfully, push as normal:
   ```
   git push
   ```

9. Submit `src/wimboot.cab` for UEFI Secure Boot signing using
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

5. Verify that the binaries have not been altered by the signing
   process:
   ```
   make -C src
   ```

6. Commit the signed binaries with a message such as:
   ```
   [release] Release version 2.7.0-signed
   ```

7. Push the signed binary branch, e.g.:
   ```
   git push origin v2.7.0-signed
   ```
   This will automatically replace the unsigned binaries and convert
   the prerelease to a full release.
