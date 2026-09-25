# Testing the game without installing anything

Every time code is pushed, GitHub Actions automatically:

1. builds the game for **Windows, macOS, Linux and the browser**,
2. runs the determinism tests on all of them,
3. (once set up below) publishes the browser version to a **web link**.

## Option A — play in your browser (recommended)

One-time setup, about 10 minutes:

1. **Create a free Cloudflare account** at <https://dash.cloudflare.com/sign-up>.
2. **Find your Account ID**: in the Cloudflare dashboard, open
   *Workers & Pages*; the Account ID is shown in the right-hand sidebar
   (or in the browser address bar after `dash.cloudflare.com/`). Copy it.
3. **Create an API token**: profile icon (top right) → *My Profile* →
   *API Tokens* → *Create Token* → *Create Custom Token*.
   - Name: `mm3 deploy`
   - Permissions: **Account → Cloudflare Pages → Edit**
   - Create, then copy the token (it's shown only once).
4. **Add both to GitHub**: open the repo on GitHub → *Settings* →
   *Secrets and variables* → *Actions* → *New repository secret*:
   - `CLOUDFLARE_API_TOKEN` = the token
   - `CLOUDFLARE_ACCOUNT_ID` = the account ID
5. Re-run the latest workflow (repo → *Actions* → latest run →
   *Re-run all jobs*) or push any change.

Your links:

- `main` branch → **https://mm3.pages.dev**
  (if that name is taken, Cloudflare shows the real name in *Workers & Pages*)
- any other branch → `https://<branch-name>.mm3.pages.dev`
  (the exact URL is printed at the end of the *deploy-web* job log)

Click the game once so it gets keyboard focus, then play.

### Keep the test link private (recommended since you plan to sell)

Anyone with the link can play it. To lock it down: Cloudflare dashboard →
*Zero Trust* (free for up to 50 users) → *Access* → *Applications* →
*Add an application* → *Self-hosted* → domain `mm3.pages.dev` (and
`*.mm3.pages.dev`) → policy "Allow" → *Emails* → your email address. You'll
then log in with a one-time code emailed to you.

## Option B — download the desktop build

1. Repo on GitHub → *Actions* tab → click the latest green run.
2. Scroll to **Artifacts** and download `mm3-windows`, `mm3-macos` or
   `mm3-linux`; unzip and run.
   - Windows may show "Windows protected your PC" (unsigned app): *More info*
     → *Run anyway*.
   - macOS: right-click → *Open* the first time (unsigned app); you may need
     `chmod +x mm3` in Terminal after unzipping.
3. `mm3-web` contains the browser files; they need a web server to run, so
   Option A is easier.

## What "determinism tests passed" means

CI plays one minute of scripted input in every physics style on every
platform and checks the results are bit-identical everywhere. If a change
breaks that, the run turns red and nothing is deployed from that job.
