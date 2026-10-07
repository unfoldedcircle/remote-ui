# Contributing

Thanks for taking the time to contribute!

Found a bug, typo, missing feature or a description that doesn't make sense or needs clarification?  
Great, please let us know!

### Bug Reports :bug:

If you find a bug, please search for it first in the [GitHub issues](https://github.com/unfoldedcircle/remote-ui/issues),
and if it isn't already tracked, [create a new issue](https://github.com/unfoldedcircle/remote-ui/issues/new/choose).

⚠️ Please don't report security vulnerabilities as public issues: follow our [security policy](SECURITY.md) instead.

### Pull Requests

**Any pull request needs to be reviewed and approved by the Unfolded Circle development team.**

We love contributions from everyone.

⚠️ If you plan to make functional changes or add new features, we kindly ask you to reach out to us first.  
Either open a feature request describing your proposed changes before submitting code, or contact us on one of the
other [feedback channels](#feedback-speech_balloon). This helps ensure your time and effort is well-invested, as
unsolicited pull requests for functional changes may not be accepted.

Since this software runs on the Remote Two and Remote 3, we have to make sure it remains compatible with the
[Core-API](https://github.com/unfoldedcircle/core-api) and the embedded runtime environment, and runs smoothly.

With that out of the way, here's the process of creating a pull request and making sure it passes the automated tests:

### Planning a change :memo:

Non-trivial changes are planned before they are implemented: see the
[development workflow](docs/workflow.md). A change is proposed under `openspec/changes/` with the
intended behaviour written as a spec delta, durable decisions are recorded in `docs/adr/`, and the
living specs under `openspec/specs/` describe what the app does today. A trivial fix does not need
a change — fix it, add the `CHANGELOG.md` entry, open the PR.

### Contributing Code :bulb:

1. Fork the repo.

2. Make your changes or enhancements on a feature branch (see the best practices below).

   Contributed code must be licensed under the GNU General Public License 3.0 or later (GPL-3.0-or-later).  
   Add the copyright notice and the SPDX license identifier to the top of each new file, as shown in the
   [code guidelines](docs/code_guidelines.md#file-header).

3. Follow the [code guidelines](docs/code_guidelines.md): format the lines you changed with clang-format and make
   the lints pass with [cpplint](https://github.com/cpplint/cpplint) and the design system check for QML:
    ```shell
    ./cpplint.sh
    ./design-check.sh
    ```

4. Make sure your changes build and the unit tests pass (`make test`, see [Testing](AGENTS.md#testing)).

5. Only check in the `en_US.ts` translation file, and only if language texts have changed.
   We are syncing the other language files from our translation service.

6. Add a `CHANGELOG.md` entry for every user-visible change, see the
   [code guidelines](docs/code_guidelines.md#changelog).

7. Push to your fork.

8. Submit a pull request.

At this point we will review the PR and give constructive feedback.  
This is a time for discussion and improvements, and making the necessary changes will be required before we can
merge the contribution.

### Pull Request Best Practices

To ensure efficient review and maintain code quality, please follow these guidelines:

- **One feature per pull request**: Each PR should address a single feature, bug fix, or improvement. This makes
  reviews faster and reduces the risk of introducing bugs. If you have multiple unrelated changes, please submit them
  as separate pull requests.

- **Clean commit history**: Before submitting, rebase or squash your commits to create a clean, logical history. Each
  commit should represent a meaningful, atomic change with a commit message that follows the
  [code guidelines](docs/code_guidelines.md#commit-messages). If your branch gets behind the main branch, please
  rebase instead of merging to avoid merge commits in your PR branch.

- **Branch names**: `feat/<short-description>` for features, `fix/<issue-number>` for bug fixes.

- **Test before submitting**: Only open a pull request once you have thoroughly tested your changes locally. Ensure
  the build, the lints and all unit tests pass. **Pull requests with failing automated checks will not be reviewed**
  until they pass.

- **Draft pull requests for early feedback**: If your work is still in progress and you're seeking early feedback, you
  may open a draft pull request. However, please only do this **upon prior agreement** with the maintainers to ensure
  reviewers have availability for interim reviews. Use the draft status to clearly indicate the work-in-progress
  nature.

- **No work-in-progress after opening (non-draft)**: Once you open a regular (non-draft) pull request, consider it
  complete. Do not continue adding new features or making unrelated changes. If additional work is needed based on
  review feedback, address only those specific points. For new features, create a new branch and PR.

- **Descriptive titles and descriptions**: Use clear, concise PR titles and provide a detailed description explaining
  what changes were made, why they were needed, and any relevant context. Describe how you tested the change, and on
  which remote model or in the desktop simulator.

### AI-Assisted Contributions :robot:

The use of AI coding assistants and Large Language Models (LLMs) is permitted, **but must be disclosed and used
responsibly**. We follow a "human-in-the-loop" policy inspired by industry best practices:

- **Disclosure required**: If you used AI tools (GitHub Copilot, Cursor, Claude, ChatGPT, etc.) to generate or assist
  with any portion of your contribution, you **must disclose this** in your pull request description. Specify which
  files or sections were AI-assisted.

- **Attribution in commits**: A commit that an AI tool wrote or helped to write names the model in a `Co-authored-by:`
  trailer, as described in the [code guidelines](docs/code_guidelines.md#commit-messages). Commit messages and pull
  requests must not contain agent session information, such as session IDs or session links.

- **Project rules for agents**: Coding agents find the project's boundaries, traps and conventions in
  [AGENTS.md](AGENTS.md). Point your agent at it.

- **You are responsible**: As the contributor, you are fully responsible for all code you submit, regardless of
  whether AI was used. This means you must:
   - Understand every line of code you submit
   - Have thoroughly tested all AI-generated code
   - Be able to explain and defend design decisions during code review
   - Fix any issues that arise from AI-generated code

- **No "AI slop"**: Low-quality, unreviewed, or poorly understood AI-generated contributions will be **rejected
  without review**. This includes:
   - Code that appears to be generated without human understanding
   - Excessive or unnecessary changes generated by AI agents
   - Code with incorrect assumptions about the codebase
   - Documentation or comments that are generic or inaccurate

- **Human review mandatory**: All AI-assisted code must be carefully reviewed, tested, and validated by you before
  submission. Do not submit code you cannot explain or maintain.

**Example disclosure statement for PR description:**

AI Assistance Disclosure: I used GitHub Copilot to assist with "specific function/file". All code has been reviewed,
tested, and I understand the implementation fully.

### Feedback :speech_balloon:

There are a few different ways to provide feedback:

- [Create a new issue](https://github.com/unfoldedcircle/remote-ui/issues/new/choose)
- [Reach out to us on Twitter](https://twitter.com/unfoldedcircle)
- [Visit our community forum](http://unfolded.community/)
- [Chat with us in our Discord channel](http://unfolded.chat/)
- [Send us a message on our website](https://unfoldedcircle.com/contact)
