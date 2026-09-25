# Business and licensing logistics

Practical checklist for keeping the game closed-source and sellable. Not
legal or tax advice.

## Keep the code private

- **Current status: the repository is public** (so GitHub Pages is free and no
  signup is needed for testing). Anyone can read and copy the code; the
  "All rights reserved" license still makes copying illegal, but it's hard to
  enforce.
- Before adding anything valuable (real art, music, the level-sharing server,
  a store build), make the repo **private** (Settings → General → Danger Zone
  → Change visibility) and move browser hosting to a service that works with
  private repos (Cloudflare Pages, Netlify, itch.io, or a paid GitHub plan).
- `LICENSE` is "All rights reserved": nobody may use the code without your
  written permission.
- Only add collaborators you trust; have contractors sign an agreement that
  assigns you the copyright in their work (code, art, music).

## Dependency license policy

Only libraries that allow closed-source commercial use without releasing your
source code:

| Allowed | Examples |
|---|---|
| ✅ zlib, MIT, BSD, Apache-2.0, Boost, public domain / CC0 | SDL2 (zlib), stb (public domain/MIT) |
| ⚠️ LGPL | Only if dynamically linked and you comply — avoid; impossible on consoles/web |
| ❌ GPL, AGPL, "non-commercial" licenses | Never |

Every dependency that ships goes into `THIRD_PARTY_NOTICES.md`.

## Where the game could be sold

| Channel | Cut | Notes |
|---|---|---|
| itch.io | you choose (default 10%) | Easiest start; supports browser games and paid downloads |
| Steam (Windows/macOS/Linux/Steam Deck) | 30% (less at high revenue) | $100 app fee per game; Steam Workshop could host levels |
| Web (own site, premium or ads) | payment processor ~3% | Needs accounts + payments backend; web builds can be copied, so keep the valuable parts (level sharing, accounts) server-side |
| Google Play / Apple App Store | 15% (small devs) – 30% | Apple needs a Mac + $99/yr; Google $25 once |
| Nintendo / PlayStation / Xbox | platform-set | Apply to their developer programs or partner with a publisher; dev kits and ratings (ESRB/PEGI) required |

## Business setup (before first sale)

- Choose a **public game name** and check it's free: trademark search
  (USPTO/EUIPO), domain name, store name availability.
- Consider forming an LLC (or local equivalent) to separate personal liability.
- Tax: stores will ask for tax forms (e.g. W-9/W-8); track income.
- Online level sharing will need: user accounts, content moderation and a
  report button, a privacy policy and terms of service, and age-rating
  questionnaires that mention user-generated content.

## Monetization ideas that fit a level maker

- Paid game (one-time purchase) — simplest and most player-friendly.
- Cosmetic packs / new themes / new physics styles as DLC.
- Optional supporter subscription for extra upload slots (no pay-to-win).
