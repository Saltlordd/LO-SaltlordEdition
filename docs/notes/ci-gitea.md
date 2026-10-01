# Pull request checks on Gitea

Since 2026-09-30 the pull request checks run on Gitea Actions at
`git.zkx.ca` instead of GitHub Actions. GitHub queues were too slow for these
short jobs. The GitHub copies under `.github/workflows/` keep only
`workflow_dispatch`, so they can still be started manually. Release packaging,
the Mod Wiki publication and issue triage stay on GitHub.

| Workflow (`.gitea/workflows/`) | Jobs | Runner |
|---|---|---|
| `fg-cpu-contracts.yml` | GPU-free FG contracts | Linux and Windows |
| `fg-game-integration.yml` | clang-cl compile of the production renderer/video units, FG bridge link and input contract | Windows |
| `reusable-fg.yml` | Shared FG core; DLSS, FSR and combined native adapters | Linux and Windows |
| `review-regressions.yml` (from the 2026-09-30 project review) | Sanitizer regression suite | Linux |

## Running the checks

Push the branch to the Gitea remote. Each workflow keeps the path filter of
its GitHub original and runs on branch pushes, Gitea pull requests or a manual
start. A newer push to the same branch cancels the older run.

```powershell
git push zkx <branch>
```

Results are on the repository's Actions page at `git.zkx.ca`. `tea` 0.14
cannot list runs on Gitea 1.25 (`tea actions` needs 1.26). Use the API instead,
for example `tea api --login zkx "/repos/freefrank/LostOdysseyRecomp/actions/jobs?limit=20"`
and `.../actions/jobs/<id>/logs`.

In the agent's non-interactive Git Bash, `tea` started directly hangs, even
for `--version`. The zkx credential helper is `tea login helper`, so a push
from there hangs too. Started from PowerShell or through `cmd //c` it works.

## Runners

| Runner | Labels | Host |
|---|---|---|
| `docker-runner` | `ubuntu-latest`, `ubuntu-24.04`, `ubuntu-22.04` | Docker on the Gitea host |
| `win-t640` | `windows-2022`, `windows-latest`, `windows` (host mode) | T640, Windows Server 2022, 64 threads |

The Linux image has no CMake or compiler, so the workflows install `cmake`,
`g++` and, where needed, `python3` and `libsdl2-dev` first.

On T640, `act_runner` runs from `C:\act_runner` as the `gitea-act-runner`
scheduled task (SYSTEM, at boot). The task runs `start-runner.cmd`, which sets
up the job environment and appends to `runner.log`. `config.yaml` sets three
concurrent jobs, `D:\act_runner\work` as the work directory, and the host
labels above. Without explicit labels, the generated default config lists
Docker labels and the runner exits looking for Docker.

T640 toolchain:

- Visual Studio 2022 Build Tools 17.14 (MSVC 14.44, Windows SDK 10.0.26100, ClangCL MSBuild toolset)
- LLVM 22.1.8 in `C:\Program Files\LLVM`, matching the development machine
- CMake 3.31.6 and PowerShell 7.4.6
- Git for Windows, Node.js, and aria2 in `C:\tools\aria2`

The VS-bundled clang component did not finish installing, so
`start-runner.cmd` sets `LLVMInstallDir` and `LLVMToolsVersion=22`, and
`-T ClangCL` uses the standalone LLVM. It also puts Git Bash first on `PATH`;
otherwise `shell: bash` resolves to the WSL `bash.exe` in System32.

### Submodule mirror

`D:\ci-cache\git\deps.git` is a bare repository holding the 15 upstreams that
the FG compile job clones: plume and its contrib libraries, XenosRecomp with
dxc-bin and its other dependencies, XenonRecomp with tomlplusplus,
unordered_dense, and SDL. Each upstream's tags live under `refs/rtags/<name>/`
so that equal tag names do not collide. The mirror was 518 MB on 2026-09-30.
The `gitea-git-cache-refresh` scheduled task fetches it every 6 hours
(`C:\act_runner\refresh-git-cache.cmd`, log `C:\act_runner\git-cache.log`).

`start-runner.cmd` exports `LO_GIT_REFERENCE=D:/ci-cache/git/deps.git`.
When it is set, the workflow passes `--reference` to `git submodule update`,
so objects come from the mirror and only newer ones from GitHub. A stale
mirror makes a job slower but does not break it. Runners without the
variable keep `--depth 1`. The mirror is owned by the setup account, so
`safe.directory` lists it in the system git configuration for the SYSTEM runner.
To add an upstream, add a remote with the same two fetch refspecs and
`tagOpt --no-tags`, then fetch.

## Differences from the GitHub workflows

- `actions/upload-artifact@v3`: Gitea rejects v4 as an unsupported GHES server.
- The pinned Streamline and FidelityFX headers come from a credential-free
  shallow sparse `git fetch`. `actions/checkout` would send the Gitea job token
  to github.com, which answers 401, and git then fails asking for a username.
  The sparse paths use cone mode, because Git Bash rewrites `/`-prefixed
  arguments into Windows paths.

## Timing

First green run, 2026-09-30, measured from job start to end:

| Job | Time |
|---|---|
| FG CPU contracts, Linux / Windows | 26 s / 80 s |
| Reusable FG core, Linux / Windows; native DLSS, FSR, both | 24 s / 26 s; 36–43 s |
| Review regressions (Linux) | 69 s |
| FG game integration compile (Windows) | 366 s; 167 s with the submodule mirror |

Without the mirror, the FG compile job spent about three minutes cloning
submodules from GitHub. With it, submodules and SDK headers took 25 s, CMake
configuration 25 s, and the build and tests 1 min 48 s.
