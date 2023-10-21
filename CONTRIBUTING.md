# Contributing

Thanks for taking the time to contribute!

Found a bug, typo, missing feature or a description that doesn't make sense or needs clarification?  
Great, please let us know!

### Bug Reports :bug:

If you find a bug, please search for it first in the [GitHub issues](https://github.com/unfoldedcircle/remote-ui/issues),
and if it isn't already tracked, [create a new issue](https://github.com/unfoldedcircle/remote-ui/issues/new/choose).

### Pull Requests

**Any pull request needs to be reviewed and approved by the Unfolded Circle development team.**

We love contributions from everyone.

⚠️ If you plan to make substantial changes, we kindly ask you, that you please reach out to us first.  
Either by opening a feature request describing your proposed changes before submitting code, or by contacting us on
one of the other [feedback channels](#feedback-speech_balloon).

Since this software is being used on the embedded Remote Two device, we have to make sure it remains
compatible with the embedded runtime environment and runs smoothly.

With that out of the way, here's the process of creating a pull request and making sure it passes the automated tests:

### Contributing Code :bulb:

1. Fork the repo.
   
2. Make your changes or enhancements (preferably on a feature-branch).

    Contributed code must be licensed under the GNU General Public License 3.0 or later (GPL-3.0-or-later).  
    It is required to add a boilerplate copyright notice to the top of each file:

    ```
    // Copyright (c) {year} {person OR org} <{email}>
    // SPDX-License-Identifier: GPL-3.0-or-later
    ```

3. Make sure your changes build.

4. Make sure your changes make the lints pass with [cpplint](https://github.com/cpplint/cpplint):
    ```shell
    cpplint.sh
    ```

5. Make sure your changes follow the project's code style.
   - Use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) with the defined configuration file: [.clang-format](.clang-format)
   - This can also be setup in Qt Creator under Preferences, Beautifier: Clang Format. Use predefined style: `File` and fallback style: `Google`.

6. Manually check code with Clang-Tidy in Qt Creator (Debug sidebar: Clang-Tidy).

7. Push to your fork.

8. Submit a pull request.  
   Make sure you only check-in & include the `en_US.ts` translation file in the pull request, if language texts have changed.
   We are syncing the other language files from Crowdin.

At this point we will review the PR and give constructive feedback.  
This is a time for discussion and improvements, and making the necessary changes will be required before we can
merge the contribution.

### Feedback :speech_balloon:

There are a few different ways to provide feedback:

- [Create a new issue](https://github.com/unfoldedcircle/remote-ui/issues/new/choose)
- [Reach out to us on Twitter](https://twitter.com/unfoldedcircle)
- [Visit our community forum](http://unfolded.community/)
- [Chat with us in our Discord channel](http://unfolded.chat/)
- [Send us a message on our website](https://unfoldedcircle.com/contact)
