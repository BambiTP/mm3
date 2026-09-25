# Testing the game without installing anything

Every time code is pushed, GitHub Actions automatically:

1. builds the game for **Windows, macOS, Linux and the browser**,
2. runs the determinism tests on all of them,
3. publishes the browser version to a **web link** per branch (after the
   one-time step in Option B).

## Option A — ask Claude for the play link (instant)

When Claude changes the game in a Claude Code session, it rebuilds the browser
version and republishes it to a **private claude.ai page** (only you can open
it). The link stays the same after every change, so you just **refresh** it.
Ask "give me the play link" if you don't have it.

## Option B — per-branch link on GitHub Pages (about 3 minutes after a push)

One-time setup, about 1 minute (after the first push has created the
`gh-pages` branch):

1. Open the repo on GitHub → **Settings** → **Pages** (left sidebar).
2. Under *Build and deployment* → *Source*, choose **Deploy from a branch**,
   then branch **gh-pages**, folder **/ (root)**, and click **Save**.

After that, every push publishes the browser version automatically:

- `main` → **https://bambitp.github.io/mm3/**
- any other branch → `https://bambitp.github.io/mm3/b/<branch-name>/`, with `/`
  in the branch name replaced by `-`. The exact link is shown on the run's
  summary page in the *Actions* tab.

These links are public, like the repo. See `docs/BUSINESS.md` for moving to
private hosting later.

### Running your own temporary link (cloudflared, no signup)

If you have the `mm3-web` files on your PC (see Option C) and
[cloudflared](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/downloads/):

```sh
cd mm3-web
python -m http.server 8000                       # terminal 1
cloudflared tunnel --url http://localhost:8000   # terminal 2, prints a trycloudflare.com link
```

The link works only while both commands keep running.

## Option C — download a build

1. Repo on GitHub → *Actions* tab → click the latest green run.
2. Scroll to **Artifacts** and download `mm3-windows`, `mm3-macos` or
   `mm3-linux`; unzip and run.
   - Windows may show "Windows protected your PC" (unsigned app): *More info*
     → *Run anyway*.
   - macOS: right-click → *Open* the first time (unsigned app); you may need
     `chmod +x mm3` in Terminal after unzipping.
3. `mm3-web` contains the browser files; they need a web server to run
   (see the cloudflared section), so Options A and B are easier.

## What "determinism tests passed" means

CI plays one minute of scripted input in every physics style on every
platform and checks the results are bit-identical everywhere. If a change
breaks that, the run turns red and nothing is deployed from that job.
