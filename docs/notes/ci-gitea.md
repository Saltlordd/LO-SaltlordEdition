# Pull request checks and releases on Gitea

Since 2026-09-30 the four short Windows/Linux pull request checks below run on
Gitea Actions at `git.zkx.ca`; GitHub queues were too slow for these short
jobs. Windows and Linux release packaging followed on 2026-10-01 (see
[Releases](#releases)). The GitHub copies of those workflows keep only
`workflow_dispatch`, so they can still be started manually. The macOS arm64
workflow is an explicit exception: on `macos-26`, pushes to `main` and pull
requests targeting `main` build libraries and tests without game data; it does
not link the complete game runtime. Full ARM64 runtime and gameplay evidence
is recorded in [development status](../STATUS.md). The Mod Wiki publication
and issue triage stay on GitHub.

| Workflow (`.gitea/workflows/`) | Jobs | Runner |
|---|---|---|
| `fg-cpu-contracts.yml` | GPU-free FG contracts | Linux and Windows |
| `fg-game-integration.yml` | clang-cl compile of the production renderer/video units, FG bridge link and input contract | Windows |
| `reusable-fg.yml` | Shared FG core; DLSS, FSR and combined native adapters | Linux and Windows |
| `review-regressions.yml` (from the 2026-09-30 project review) | Sanitizer regression suite; updater AppImage restart test; AppImage and Flatpak packaging script tests | Linux |

## Running the checks

Push the branch to the Gitea remote. Each workflow keeps the path filter of
its GitHub original and runs on branch pushes, Gitea pull requests or a manual
start. A newer push to the same branch cancels the older run.

```powershell
git push zkx <branch>
```

A push of a new branch whose commits all exist on other branches (for
example a branch that only merges others) can start no run. Start the
workflows manually in that case:

```powershell
tea api --login zkx -X POST -d '{"ref":"<branch>"}' /repos/freefrank/LostOdysseyRecomp/actions/workflows/<workflow>.yml/dispatches
```

Results are on the repository's Actions page at `git.zkx.ca`. `tea` 0.14
cannot list runs on Gitea 1.25 (`tea actions` needs 1.26). Use the API instead,
for example `tea api --login zkx "/repos/freefrank/LostOdysseyRecomp/actions/jobs?limit=20"`
and `.../actions/jobs/<id>/logs`.

Each workflow ends with a `github-status` job that writes the overall result
to the GitHub commit as the status `gitea/<workflow>`, linked to the Gitea run,
so GitHub pull requests show the outcome. It needs the Gitea repository secret
`LO_GITHUB_TOKEN`: a fine-grained GitHub token for the owner's repositories with
"Commit statuses: Read and write" and, for the release workflow, "Contents:
Read and write". Without the secret the job skips. A commit that has not been
pushed to GitHub yet gets no status (HTTP 422 in the job log).

In the agent's non-interactive Git Bash, `tea` started directly hangs, even
for `--version`. The zkx credential helper is `tea login helper`, so a push
from there hangs too. Started from PowerShell or through `cmd //c` it works.
A PowerShell wrapper script around `tea` must not bind `-d` as a script
parameter: PowerShell takes it as `-Debug`, and the request goes out without a
body (Gitea answers "Empty Content-Type").

## Releases

`.gitea/workflows/release.yml` is the port of the GitHub release workflow. A
`v*` tag pushed to `zkx` starts it, and so does a manual start with an
optional `release_tag`. Push the tag to GitHub first: the draft step runs
`gh release create --verify-tag`, which needs the tag there.

```powershell
git push origin refs/tags/vX.Y.Z
git push zkx refs/tags/vX.Y.Z
```

| Job | Runner |
|---|---|
| Create draft release (before the builds, so a missing CHANGELOG section or tag fails early) | `docker-runner` |
| FSR shader inputs, Windows build and ZIP | `win-t640` |
| Linux build, AppImage and Flatpak | `docker-lo-release-privileged` |
| Publish: upload the Gitea artifacts to the draft, verify the set, make it public and latest | `docker-runner` |

The build jobs only produce Gitea artifacts. The publish job runs after both
platforms have built, downloads the three package artifacts and uploads the
ones the release does not have yet, so a failed platform leaves the draft
without a partial package set. Unlike the GitHub workflow, the build jobs do
not upload to the release themselves.

Since PR #127, both build jobs fetch the pinned Vulkan shader pack before
compiling and pass it as `LO_PORTABLE_SHADER_PACK`; the runtime build runs
`LoShaderPackTool verify-runtime` against the private disc 1 image and fails
if the pack does not match ([refresh procedure](../PORTABLE_SHADER_PACK.md#runtime-contract-and-release-check-after-v0725)).
Build-only run 485 printed `runtime_compatibility_verified: true` on both.

The Gitea job token cannot reach GitHub, so every GitHub operation uses
`LO_GITHUB_TOKEN` with `GH_REPO` pinned to `freefrank/LostOdysseyRecomp`:
creating the draft, uploading the packages, publishing, downloading the pinned
Streamline SDK, and fetching the private `freefrank/LostOdysseyRecomp-build-inputs`
commits (no deploy key on Gitea). A manual start without `release_tag` builds
the selected branch and keeps the packages as Gitea artifacts; it writes
nothing to GitHub. To package an existing tag again, start it with that tag:

```powershell
tea api --login zkx -X POST -d '{"ref":"main","inputs":{"release_tag":"vX.Y.Z"}}' /repos/freefrank/LostOdysseyRecomp/actions/workflows/release.yml/dispatches
```

As on GitHub, an existing release keeps its reviewed notes and assets; only
missing assets are uploaded, and a public release stays as it is.

First release through this workflow, 2026-10-01: v0.7.25 (PR #115 merged as
`e3ad1b9`; tag commit `2c65f0b`) was the first release built and published by
it. Pushing the tag to `zkx` started [run 67](https://git.zkx.ca/freefrank/LostOdysseyRecomp/actions/runs/67),
and all five jobs succeeded: create draft 0.2 min, FSR inputs 1.9 min, Windows
8.8 min (`win-t640`), Linux AppImage and Flatpak 13.6 min (privileged docker
runner) and publish 2.9 min, which downloaded the Gitea artifacts, uploaded
them to the GitHub draft, verified the set and published it. About 19 minutes
passed from the tag push to publication at 2026-10-01T08:04:23Z. It was also
the first real GitHub write (draft creation, upload, publication) through
`LO_GITHUB_TOKEN`; earlier runs were build-only or the read-only v0.7.20
rehearsal (run 55). A separate check of the published Windows ZIP found its
SHA-256 equal to GitHub's digest and a manifest with version `v0.7.25`; see
[STATUS](../STATUS.md#v0725-published--2026-10-01). A green run shows that the
pipeline works, not that the release has been played or accepted.

Differences from the GitHub release workflow:

- Flatpak needs bubblewrap, which needs a privileged container, hence the
  dedicated runner. `gh` 2.63.2 is downloaded in each Linux job.
- Windows jobs use T640's Python 3.12 (a venv for the packaging tools) instead
  of `setup-python`, 24 build jobs and 16 FSR shader threads. The Linux build
  uses 20 jobs. The Linux VM runs on T640 too, so the two builds share its
  64 threads; a Linux compiler process peaked below 0.5 GB.
- Compiler cache: both builds run compilers through ccache (4.14.1 in
  `C:\tools\ccache` on T640, the distribution package on Linux) with
  `CCACHE_DIR=$LO_CI_CACHE/ccache` and `CCACHE_BASEDIR` set to the workspace, so
  hits do not depend on the per-job directory. The generated PPC units only
  change with the XEX or the recompiler, and they took about 9 of the 11
  minutes of the first Linux build. The job log ends the build with
  `ccache --show-stats`. Translation units that use the runtime's precompiled
  header fall back to the compiler. On Windows only the C units hit (212 of
  837 compilations): the release build uses the Visual Studio clang-cl 19.1.5
  that `vcvars64` puts first on `PATH`, as the GitHub runners did, and CMake
  passes C++23 to it as `-clang:-std=c++23`, an option ccache rejects. clang-cl
  20+ accepts `/std:c++23preview`, which ccache caches and which produced
  byte-identical objects on clang-cl 22 with `/Brepro`; using it would mean
  building releases with LLVM 22 instead. The Windows build takes about
  4 minutes with 24 jobs and runs beside the longer Linux job.
- Runner caches: both release runners set `LO_CI_CACHE` (`D:\ci-cache` on
  T640; `/ci-cache` on the privileged runner, the `lo-release-cache` docker
  volume). `fetch_dlss_sdk.py` keeps a verified checkout of the pinned SDK in
  `nvidia-dlss-<commit>` there and copies it instead of fetching about
  0.5 GB again; a damaged entry is dropped and fetched anew. The Windows job
  keeps the Streamline archive in `streamline/` once it has passed the size
  and SHA-256 check. Windows submodules use the `LO_GIT_REFERENCE` mirror. The
  privileged runner also keeps `/var/lib/flatpak` in the `lo-release-flatpak`
  volume; the job runs `flatpak update` for the two runtimes so a kept copy
  matches a fresh install. The private game input is not cached. Without
  `LO_CI_CACHE` every step downloads as before.
- On the Windows host runner, a second `actions/checkout` in the same job fails:
  act re-fetches its cached copy of the action, and Windows denies access to
  the replaced pack file. The FidelityFX SDK and the private inputs are
  fetched with plain git; the token reaches git through `GIT_CONFIG_*`
  variables, so it is neither on a command line nor in `.git/config`.

## Runners

| Runner | Labels | Host |
|---|---|---|
| `docker-runner` | `ubuntu-latest`, `ubuntu-24.04`, `ubuntu-22.04` | Docker on the Gitea host |
| `docker-lo-release-privileged` | `ubuntu-22.04-privileged` (this repository only) | Docker on the Gitea host, privileged containers |
| `win-t640` | `windows-2022`, `windows-latest`, `windows` (host mode) | T640, Windows Server 2022, 64 threads |

The Gitea host (`192.168.1.7`) is a Linux VM on T640 with 32 threads and
31 GB of memory, about half of it free for jobs. The privileged runner is a
separate compose project in `/root/app/gitea-runner-lo` (container
`gitea-runner-lo-release`, one job at a time). It is registered to this
repository only, because a privileged job container can control the Docker
host. Only the release workflow uses its label. Its `data/config.yaml` sets
`LO_CI_CACHE: /ci-cache` under `runner.envs`, mounts the `lo-release-cache`
and `lo-release-flatpak` volumes through `container.options`, and lists both
in `container.valid_volumes`.

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
- Python 3.12.10 in `C:\Program Files\Python312`, unpacked from the official NuGet package because the python.org installer fails on this machine, as `setup-python` did in 2026-09
- Git for Windows, Node.js, and aria2 in `C:\tools\aria2`
- GitHub CLI 2.63.2 in `C:\tools\gh` (on the runner `PATH`), used by the release workflow

`start-runner.cmd` also sets `MSBUILDDISABLENODEREUSE=1`, so idle MSBuild
worker processes do not linger and hold files between jobs.

Until 2026-10-01, T640 had TCP receive window auto-tuning and RSS disabled.
That capped every single TCP connection at 64 KB per round trip, about
1.9 MB/s to GitHub, while multi-connection tests such as fast.com still
showed 400 Mb/s. The first Gitea release run spent 14 minutes fetching the
DLSS SDK. With `netsh int tcp set global autotuninglevel=normal` and
`rss=enabled`, one connection reaches about 12 MB/s. If GitHub downloads on
T640 become slow again, check `netsh int tcp show global` first.

The VS-bundled clang package stalled for half an hour during setup (it
completed later), so `start-runner.cmd` sets `LLVMInstallDir` and
`LLVMToolsVersion=22`, and `-T ClangCL` uses the standalone LLVM 22, the same
version as the development machine. Ninja builds that run `vcvars64` first,
such as the release build, find the Visual Studio clang-cl 19.1.5 (installed
since) ahead of it on `PATH`, like the GitHub Windows runners.
`start-runner.cmd` also puts Git Bash first on `PATH`;
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

It also exports `LO_CI_CACHE=D:\ci-cache` for the release caches described
under [Releases](#releases). Entries are named by the pinned version or
commit, so a pin change adds a new entry; delete old ones by hand.

## Differences from the GitHub workflows

- `actions/upload-artifact@v3`: Gitea rejects v4 as an unsupported GHES server.
- The pinned Streamline and FidelityFX headers come from a credential-free
  shallow sparse `git fetch`. `actions/checkout` would send the Gitea job token
  to github.com, which answers 401, and git then fails asking for a username.
  The sparse paths use cone mode, because Git Bash rewrites `/`-prefixed
  arguments into Windows paths. With an explicit `token:` and
  `github-server-url: https://github.com`, `actions/checkout` works for GitHub
  repositories; the Linux release job uses that.

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
