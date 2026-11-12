# AAA Quality Standard — Mandatory for All Development

**Project-wide objective:** Triad Drift must strive for the highest achievable AAA-quality presentation across gameplay, visuals, animation, environments, UI/UX, sound design, music, and overall player experience.

AAA quality is our **development direction and acceptance standard**, not a claim that every asset must reach commercial AAA production complexity during a four-day game jam.

All developers and AI agents must follow these requirements.

Preserve Triad Drift's original bright, colorful sci-fi identity and its approved Rift magenta, Lancer cyan, and Bastion amber presentation.

## 1. Incremental Quality Development

Every significant feature or asset should progress through the following stages:

1. **Implement:** Establish the intended functionality or visual/audio foundation.
2. **Verify:** Confirm correct operation, stability, and compatibility with existing systems.
3. **Refine:** Improve realism, presentation, responsiveness, and overall quality.
4. **Retest:** Verify improvements haven't introduced regressions or performance problems.
5. **Review:** Obtain human approval for significant subjective visual, audio, or gameplay decisions.
6. **Integrate:** Safely incorporate approved work into the playable game.

**Never treat a functioning prototype as automatically production-ready.**

Technical verification and presentation approval are separate requirements.

## 2. Quality Standards by Discipline

**Gameplay and Combat**

- Responsive controls, satisfying combat feedback, and consistent mechanics.
- Clear communication of damage, abilities, targeting, and player actions.
- Convincing movement, animation, and interaction.
- Prioritize enjoyable gameplay over unnecessary complexity.

**Ships, Characters, and 3D Assets**

- Cohesive silhouettes, believable construction, and refined geometry.
- High-quality materials, surface detailing, and appropriate texture resolution.
- Natural mechanical integration and convincing animation.
- Optimized, game-ready assets without unnecessary polygon or texture overhead.

**Environments, Lighting, and VFX**

- Cinematic, believable environments with convincing scale and depth.
- Natural-looking nebulae, stars, planetary surfaces, and atmospheric effects.
- High-quality lighting, material response, and controlled visual effects.
- Use real-world references where appropriate while preserving the game's original artistic identity.
- Avoid obvious procedural artifacts, excessive bloom, and visually distracting effects.

**UI/UX and HUD**

- Modern, polished, intuitive interface design.
- Critical information must be immediately recognizable during gameplay.
- Consistent typography, spacing, visual hierarchy, and animation.
- Clear health, shields, abilities, cooldowns, and targeting feedback.
- Maintain readability across supported resolutions.

**Audio and Music**

- Cinematic, immersive, professionally balanced sound.
- Distinctive, believable weapons, engines, impacts, and environmental effects.
- Clean attacks, natural fades, appropriate dynamic range, and controlled harshness.
- Original, cohesive music supporting gameplay intensity and emotional progression.
- Avoid placeholder-quality, unnecessarily abrasive, or retro-style audio unless intentionally approved.

## 3. Quality Acceptance Gates

Before declaring a significant feature or asset complete, evaluate:

- **Functionality:** Does it work correctly?
- **Stability:** Does it pass appropriate verification?
- **Presentation:** Does it meet the current approved quality target?
- **Clarity:** Can the player understand and respond to it?
- **Performance:** Is it suitable for real-time gameplay?
- **Integration:** Does it function correctly in the actual game?
- **Human Review:** Has subjective quality been approved when required?

Reports must distinguish between:

- Prototype complete.
- Technically verified.
- Presentation approved.
- Integrated.
- Gameplay validated.

Do not claim AAA quality or final production readiness based solely on automated tests, screenshots, or isolated previews.

## 4. Continuous Improvement Without Scope Creep

AAA quality must be approached through **small, measurable, verified improvements**.

- Fix functional defects before adding decorative polish.
- Prioritize the most noticeable weaknesses in the player's actual experience.
- Preserve working systems when improving their presentation.
- Reuse existing assets, tools, and research before rebuilding.
- Avoid repeated unsuccessful iterations without reassessing the underlying design.
- Prefer cohesive, polished gameplay over disconnected high-detail prototypes.
- Maintain consistent quality across systems rather than perfecting one asset while others remain unfinished.

## 5. Performance and Deadline Protection

The November 15, 2026 submission deadline remains mandatory.

- Keep `main` playable and integration-ready.
- Protect time for gameplay testing, packaging, and submission.
- Avoid unnecessary high-cost rendering, simulations, and asset complexity.
- Evaluate visual improvements against performance and memory constraints.
- Do not introduce instability solely to achieve higher visual fidelity.
- Escalate major scope, quality, or schedule tradeoffs to the human developers.

**The priority is to deliver the most polished, cohesive, stable, and enjoyable complete game possible within the available time.**

## 6. Team-Wide Enforcement

These standards apply to all human developers and AI agents working on Triad Drift.

Each agent must:

- Apply the quality standard to its assigned responsibilities.
- Identify the next highest-impact improvement after technical verification.
- Report quality limitations honestly.
- Respect established ownership and integration procedures.
- Coordinate cross-team dependencies through the approved communication process.
- Request human review before major subjective design changes.

This document supplements existing `AGENTS.md`, `features.md`, `CLAIMS.md`, and `MESSAGEBOARD.md` requirements. It does not override established ownership, Git safety, communication protocols, or the agreed gameplay scope.

**Guiding principle: Build it, verify it, refine it, verify again, and integrate safely.**
