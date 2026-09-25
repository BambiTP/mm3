# Testing the game without installing anything

Every time code is pushed, GitHub Actions automatically:

1. builds the game for **Windows, macOS, Linux and the browser**,
2. runs the determinism tests on all of them,
3. publishes the browser version from `main` to a **web link** (after the
   one-time step in Option A).

## Option A — permanent link (GitHub Pages, no signup)

One-time setup, about 1 minute:

1. Open the repo on GitHub → **Settings** → **Pages** (left sidebar).
2. Under *Build and deployment* → *Source*, choose **GitHub Actions**.

That's it. Every push to the `main` branch now publishes the browser version to:

**https://bambitp.github.io/mm3/**

Click the game once so it gets keyboard focus, then play. Branches other than
`main` are not published here; use Option B to test those.

Note: the link is public, like the repo. See `docs/BUSINESS.md` for moving to
private hosting later.

## Option B — instant temporary link (cloudflared quick tunnel, no signup)

A quick tunnel gives a random `https://<words>.trycloudflare.com` link to a
build running on some computer. The link works only while that computer keeps
running the tunnel, and anyone who has the link can open it.

- **From a Claude cloud session:** ask Claude to "give me a cloudflared link".
  The session's network policy must allow `api.trycloudflare.com` and
  Cloudflare's tunnel servers (`*.argotunnel.com`, port 7844). Otherwise use
  Option A.
- **From your own PC** (after downloading the `mm3-web` artifact, see Option C,
  and [cloudflared](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/downloads/)):

  ```sh
  cd mm3-web
  python -m http.server 8000          # terminal 1
  cloudflared tunnel --url http://localhost:8000   # terminal 2, prints the link
  ```

## Option C — download a build

1. Repo on GitHub → *Actions* tab → click the latest green run.
2. Scroll to **Artifacts** and download `mm3-windows`, `mm3-macos` or
   `mm3-linux`; unzip and run.
   - Windows may show "Windows protected your PC" (unsigned app): *More info*
     → *Run anyway*.
   - macOS: right-click → *Open* the first time (unsigned app); you may need
     `chmod +x mm3` in Terminal after unzipping.
3. `mm3-web` contains the browser files; they need a web server to run
   (see Option B), so Option A is easier.

## What "determinism tests passed" means

CI plays one minute of scripted input in every physics style on every
platform and checks the results are bit-identical everywhere. If a change
breaks that, the run turns red and nothing is deployed from that job.
