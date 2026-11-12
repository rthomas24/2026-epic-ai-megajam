# Agent Modeling Strategies: How LLM/VLM Agents Produce High-Quality 3D Models by Writing Blender Python (bpy)

Compiled 2026-11-12 for the Triad Drift jam (Blender 5.2.2 LTS, official Blender Lab MCP server v1.0.1, Claude Code / Codex / Cursor, UE5 target). Evidence tags: **[PR]** peer-reviewed venue · **[PP]** arXiv preprint, peer review not verified · **[BLOG]** practitioner or vendor write-up · **[ANEC]** community anecdote or unvalidated community skill file · **[OLD]** published before 2025 · **[2°]** figure from a secondary summary, not checked against the primary text. Bullets under "Inferences" are my synthesis for skill-writing, not claims made by the sources. Numeric defaults marked *(proposed)* are untested starting values.

---

## Q1. What do LL3M, BlenderGym, BlenderAlchemy, SceneCraft, 3D-GPT, BlenderLLM, MeshCoder and 2025–2026 successors find makes LLM-written Blender code produce better assets? (loop designs and numbers)

### Takeaway
The measurable gains come from five levers:
1. Staged, part-by-part construction under a planner.
2. Grounding code in current Blender API docs or in retrieved working examples. API drift is the number one failure on Blender 5.x.
3. Feeding execution errors back to the model. Two retries lift executability from about 0.69 to about 0.97.
4. Verification that goes beyond one VLM glance: deterministic engine-state audits, checks that each critique was actually applied, yes/no question checklists, and pairwise comparisons that always keep the current best.
5. Reusable function libraries.

Free-form VLM self-critique on its own yields tiny or even negative gains. VLM judges agree with humans only about 66% of the time on which of two edits is better. Treat visual critique as one input, alongside measured checks and a revert-to-best rule.

### Cited Findings

#### LL3M: Large Language 3D Modelers (U. Chicago, arXiv 2508.08228, Aug 2025) [PP]
- **Loop.** Six agents (planner, retrieval "BlenderRAG", coding, critic, verification, user proxy) are orchestrated in AutoGen with a shared context. The planner splits the prompt into subtasks and never executes code. The coding agent receives the existing script so it edits rather than rewrites. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Phases.**
  1. Initial creation: planner → retrieval → coding, subtask by subtask, until execution succeeds.
  2. Auto-refinement: critic → coding → verification, looping until the verification agent approves. There is no fixed iteration count.
  3. User-guided refinement.
  — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Critic setup.** The critic gets **m = 5 rendered views**, with camera distance adapted to the object's bounding box, rendered in **Eevee** to save time. It looks for problems such as disconnected geometry or wrong proportions and proposes targeted fixes. The verification agent re-renders and checks each critique item was applied, and sends unapproved results back to the coder. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Models.** Planner and retrieval use GPT-4o. The coder is Claude 3.7 Sonnet. Critic and verification use Gemini 2.0 Flash per Sec. 4.1; the appendix instead lists GPT-4o with Gemini inside tools, so the paper is internally inconsistent. Runs on Blender 4.4. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Docs retrieval effect.** BlenderRAG holds 1,729 Blender 4.4 doc files. Averaged over 17 objects:
  - "Complex operations" per script: **5.86 with RAG vs 1.20 without** (about 5×).
  - Execution errors: **2.43 vs 3.29** (about 26% fewer).
  - Without RAG the coder sometimes deleted failing lines instead of fixing them.
  - Example of API drift: the Principled BSDF input "Specular" became "Specular IOR Level" in 4.x.
  — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Timing** (33 generations): about 4 min initial creation, 6 min auto-refinement, 38 s per user edit, and 10 min for a full regeneration. About 59% of user edits needed a single instruction; the rest needed 3–4 follow-ups. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Failure modes.**
  - Parts left unattached or floating after initial creation (watering-can handles, chair legs). Auto-refinement fixed some; the user fixed the rest.
  - VLM critics still miss spatial artifacts.
  - The coder applies only some critiques per pass; the verifier catches this.
  - Without the shared code context, the coder regenerates a substantially different asset.
  — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Code style that made editing work.** Comments, descriptive variable names, loops driven by parameters (e.g., number of piano keys), procedural shader-node materials, bmesh, Bézier curves, modifiers, geometry-node setups, and parent–child hierarchies. Changing one parameter changed only the intended property. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Not reported.** The agent ablations are qualitative only (figures). There are no success rates, cost figures, user-study numbers, or separate quantitative ablations of the critic or verifier. — [LL3M](https://arxiv.org/html/2508.08228v1)

#### BlenderGym (Stanford, CVPR 2025) [PR]
- **Benchmark.** 245 start→goal scene pairs across 5 tasks: procedural geometry 50, lighting 40, procedural material 40, blend shape 75, object placement 40. Each pair has at least 3 fixed camera views. Two go to the VLM, one of which must be an elevated "comprehensive" view, and every object appears in at least one input view. — [BlenderGym](https://arxiv.org/html/2504.01786v1); [CVPR page](https://openaccess.thecvf.com/content/CVPR2025/html/Gu_BlenderGym_Benchmarking_Foundational_Model_Systems_for_Graphics_Editing_CVPR_2025_paper.html)
- **Loop** (inherited from BlenderAlchemy).
  - A "brainstormer" names the most significant visual difference and proposes up to 5 edit instructions; a code editor writes before/after code edits. This runs *b* times per iteration.
  - A verifier compares pairs of concatenated renders against the goal in a knockout tournament.
  - Tree size: depth **d = 3**, breadth **b = 4**. The same VLM acts as generator and verifier.
  — [BlenderGym](https://arxiv.org/html/2504.01786v1)
- **Human vs VLM gap** (photometric loss, lower is better). Humans were 10 average Blender users with an 8-minute limit. The paper concludes graphics editing remains unsolved for VLMs. — [BlenderGym](https://arxiv.org/html/2504.01786v1)

  | Task | Human | Best VLM |
  |---|---|---|
  | Blend shape (×10⁻³) | 0.934 | 9.140 |
  | Placement (×10⁻³) | 0.423 | 10.94 |
  | Geometry (×10⁻²) | 1.269 | 6.747 |

- **How reliable VLM judges are.** On 7,950 pairwise judgments from 50 people:
  - Claude 3.5 Sonnet agreed with humans **0.66** of the time, the best VLM (random is 0.5, human–human is 0.79).
  - Qwen showed position bias, preferring the second candidate regardless of order.
  - Only Claude 3.5 Sonnet consistently gave complete reasoning.
  — [BlenderGym](https://arxiv.org/html/2504.01786v1)
- **Scaling the verifier.** Re-selecting among 32 candidates (a 4×8 tree) with more verifier passes (k from 1 to 64) improved all three metrics steadily. A scaled 8B open model beat unscaled GPT-4o and Claude 3.5 Sonnet. — [BlenderGym](https://arxiv.org/html/2504.01786v1)
- **Splitting compute between generating and verifying** (Table 2). About US$0.45 per 100 queries for the open model used. — [BlenderGym](https://arxiv.org/html/2504.01786v1)

  | Total VLM queries | Best share spent on verification | Meaning |
  |---|---|---|
  | Under 30 | 0.33 | Spend most on generating candidates |
  | 30–100 | 0.62 | Balanced |
  | Over 100 | 0.73 | Spend most on verifying |

- **Generator failure modes.**
  - Misses subtle differences, or hallucinates differences that aren't there.
  - Writes non-executable scripts, especially for node-wired materials and geometry. Open-source models failed to run more than 75% of geometry instances.
  - Writes code edits that don't match the difference it identified, especially in scripts longer than about 80 lines.
  - The metrics miss physical plausibility (a ball stuck on a basket scored well).
  — [BlenderGym](https://arxiv.org/html/2504.01786v1)

#### BlenderAlchemy (Stanford, ECCV 2024) [PR][OLD]
- **Loop.**
  - A GPT-4V edit generator proposes *b* program variants, conditioned on the current render and the user's intent.
  - A GPT-4V state evaluator picks the better of two renders in a tournament (O(log b) comparisons).
  - "Visual imagination": DALL·E 3 generates a target image from a text-only intent, and both generator and evaluator use it as the reference.
  — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1)
- **Search.**
  - *d* iterations × *b* candidates.
  - Odd iterations make "tweak" edits (numeric changes near the current best). Even iterations make "leap" edits (structural changes).
  - **The current best is always in the tournament**, so the search reverts when a whole iteration is worse.
  - Main configuration: 4×8 = 32 generator calls.
  — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1)
- **Results** (material editing).
  - CLIP ViT-B/32: **28.2 vs BlenderGPT 25.2** (ViT-L/14: 24.0 vs 21.1).
  - At a fixed 32 calls, balanced trees won: 4×8 scored photometric 0.049 / LPIPS 0.52, and 8×4 scored 0.050 / 0.50. Both beat 1×32 (0.066 / 0.64) and 32×1 (0.087 / 0.59). The authors call 4×8 to 8×4 the sweet spot.
  — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1)
- **Ablations by refinement step** (CLIP score over edits 1–4). Leap-only edits diverge from the intent; tweak-only edits plateau. — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1)

  | Variant | Edits 1→4 | Trend |
  |---|---|---|
  | Full system | 27.8 → 27.9 → 28.4 → 28.2 | Peaks at edit 3 |
  | No revert | 27.1 → 26.4 → 26.6 → 25.8 | Degrades |
  | No vision | 27.4 → 25.7 | Degrades |
  | No imagination | about 26.8–27.1 | Flat |

- **Cost and failures.**
  - Just under US$3 per material at 4×8 with GPT-4V.
  - GPT-4V sometimes imported non-existent libraries or assigned wrong-shaped values; rejection sampling handled this.
  - No reusable skill library. Edits existing scenes only. Lighting results are qualitative.
  — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1)

#### SceneCraft (ICML 2024) [PR][OLD]
- **Pipeline.**
  1. An LLM decomposes the query into sub-scenes.
  2. CLIP retrieves the top-10 candidate assets per description, renders each, and keeps the best.
  3. A planner builds a scene graph of relations (11 types, including alignment, parallelism, proximity, symmetry and repetition).
  4. A coder writes Blender code plus one scoring function per relation (score 0–1). Layout is optimized to maximize the total score.
  5. Blender renders the scene.
  — [SceneCraft](https://arxiv.org/html/2403.01248v1)
- **Two loops.**
  - Inner loop: GPT-4V reviews the render against the query, names unmet constraints, and edits the graph, functions or code. The iteration count isn't stated.
  - Outer loop ("library learning"): refined scoring functions are merged into a shared, non-parametric Python library. It was seeded with 20 hand-written examples; 20 of the 40 synthetic queries built the library and 20 were held out.
  — [SceneCraft](https://arxiv.org/html/2403.01248v1)
- **Table 1** (CLIP similarity / constraint score). The ablations appear to be cumulative. — [SceneCraft](https://arxiv.org/html/2403.01248v1)

  | Configuration | CLIP sim | Constraint score |
  |---|---|---|
  | Full | 69.8 | 88.9 |
  | No learned library | 48.3 | 64.5 |
  | Also no render-critique loop | 32.8 | 26.1 |
  | Also no relation graph | 19.4 | 3.2 |
  | BlenderGPT | 24.7 | 5.6 |

- **Human preference vs BlenderGPT** (22 responses, 10 pairs): 76.8% text fidelity, 83.6% composition, 74.5% aesthetics. Caveats: the evaluation is tiny, and the introduction's improvement claims don't match the table. — [SceneCraft](https://arxiv.org/html/2403.01248v1)

#### 3D-GPT (arXiv 2310.12945, 2023) [PP][OLD]
- **Three agents.**
  - A task-dispatch agent picks the needed procedural functions from a library, given information on all available functions.
  - A conceptualization agent enriches the user's description.
  - A modeling agent infers function parameters and writes Python that drives Blender.
- **Design.** The LLM sets the parameters of pre-built procedural generators instead of writing raw geometry.
- — [3D-GPT](https://arxiv.org/html/2310.12945v2)

#### BlenderLLM (arXiv 2412.14203, Dec 2024) [PP][OLD]
- **Data and benchmark.** BlendNet has 8,000 instruction→bpy samples; CADBench is the evaluation suite.
- **Self-improvement.** The loop generates scripts, validates them, and retrains only on those that pass. Four iterations are shown qualitatively.
- **Results.** The authors report a much lower syntax-error rate than other models. Exact numbers were not retrieved.
- — [BlenderLLM](https://arxiv.org/html/2412.14203v1)

#### MeshCoder (NeurIPS 2025) [PR]
- **Approach.**
  - Builds an expressive, higher-level Blender Python API for complex geometry.
  - Builds a large paired object↔code dataset, decomposed into semantic parts.
  - Trains a multimodal LLM that predicts code per part; rules then join the parts into one script.
  - Shapes become editable by changing code.
  — [MeshCoder](https://arxiv.org/html/2508.14879v1); [NeurIPS 2025](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0d094d83efa5ea99f86527ac7207e023-Abstract-Conference.html)
- **Results.** Average IoU 86.75% vs PLAD 67.62% vs Shape2Prog 45.03% [2°]. — [Liner summary](https://liner.com/ko/review/meshcoder-llmpowered-structured-mesh-code-generation-from-point-clouds)

#### ShapeCraft (NeurIPS 2025) [PR]
- **Representation.** A "Graph-based Procedural Shape" (GPS): the prompt is parsed hierarchically into a graph of sub-tasks, and each node stores a program snippet plus layout information.
- **Loop.** Agents iteratively refine the modeling, then "painting" with component-aware BRDF materials.
- **Results.** The authors claim better geometric accuracy than earlier LLM agents. Numbers were not retrieved.
- — [ShapeCraft](https://arxiv.org/html/2510.17603v1); [NeurIPS 2025](https://neurips.cc/virtual/2025/poster/115664)

#### L3GO (arXiv 2402.09052; NAACL 2025 demo) [PR][OLD]
- **Approach.** Part-by-part assembly in SimpleBlenv, a Blender wrapper that exposes building blocks through API calls, with environment feedback after each step.
- **Results.** Beat GPT-4, ReAct-B and Reflexion-B on ShapeNet (judged by humans and by GPT-4V). Also beat text-to-2D and text-to-3D baselines on the UFO benchmark of "unconventionally feasible" objects (e.g., a chair with five legs).
- — [L3GO](https://arxiv.org/abs/2402.09052); [ACL Anthology](https://aclanthology.org/2025.naacl-demo.37)

#### CADCodeVerify (ICLR 2025; CadQuery rather than Blender, but the same loop pattern) [PR]
- **Loop.**
  1. The VLM writes binary yes/no verification questions from the description, using few-shot example questions.
  2. It answers them from renders of the generated object.
  3. Only the "No" items are fed back for refinement.
  4. The loop stops when every answer is "Yes".
- **Results** (with GPT-4): **−7.30% point-cloud distance** and **+5.0% success rate** (the introduction says +5.5%). Benchmark: CADPrompt, 200 prompts.
- **Ablation.** Both the few-shot questions and the reference images were critical.
- — [CADCodeVerify](https://arxiv.org/abs/2410.05340v1); [ICLR 2025 paper](https://proceedings.iclr.cc/paper_files/paper/2025/file/81a934cd364e18ea6fdeaf57a93c17d4-Paper-Conference.pdf)

#### VIGA: Vision-as-Inverse-Graphics Agent (arXiv 2601.11109, Jan 2026) [PP]
- **Loop.** write → run → render → compare → revise.
  - Generator tools: `make_plan`, `execute_code`, `get_scene_info`, `get_better_assets`, `end_process`.
  - Verifier tools: `initialize_viewpoint` (puts cameras at the **four corners of the target's bounding box**), `set_camera`, `investigate` (natural-language zoom and move), `set_visibility`, `set_keyframe`, `get_scene_info`.
  - The verifier returns a description of visual differences plus a suggested next edit.
  — [VIGA](https://arxiv.org/html/2601.11109v2)
- **Memory and budgets.**
  - Memory holds plans, code diffs and render history in a sliding window (context_window = 12 in qualitative runs).
  - Up to **10 rounds** in quantitative runs and 100 in qualitative ones.
  - Best-of-N candidates are chosen by CLIP score.
  — [VIGA](https://arxiv.org/html/2601.11109v2)
- **Results** (the authors' own numbers).
  - +35.32% on BlenderGym and +124.70% on BlenderBench (30 tasks) over one-shot baselines.
  - GPT-4o material photometric loss: 3.04 vs BlenderAlchemy's 9.47 (best-of-4).
  - Gains were largest for weaker models.
  — [VIGA](https://arxiv.org/html/2601.11109v2)
- **Limitations.** GPT-4o struggled to move the camera to see the whole scene. There is no ablation of memory or round count. — [VIGA](https://arxiv.org/html/2601.11109v2)

#### SimWorlds (arXiv 2607.01766, Jul 2026) [PP]
- **Roles.**
  - Planner: writes a strategic JSON plan, then a tactical plan per stage, and decides advance / retry / replan / abort. It never operates Blender.
  - Coder: gets a fresh session per stage and writes bpy run through the MCP execute tool.
  - Per-stage reviewer: judges rendered previews plus a structured readout of the scene.
  - Final reviewer: one-shot judgment of the output against the plan.
  - All roles use claude-opus-4-7, with Blender 5.1 behind an MCP server. A scene takes about 70–120 min on an M4 with 16 GB.
  — [SimWorlds](https://arxiv.org/html/2607.01766v1)
- **Fixed stage order:** modeling → UV → texture → deformation setup → motion → camera → light → render. — [SimWorlds](https://arxiv.org/html/2607.01766v1)
- **Deterministic verifier.**
  - Structural: collection tiers, a root Empty per object, parent chains, naming, and a ground object.
  - Geometric, using BVH tests: declared contacts must be within tolerance, and **undeclared interpenetrations are hard failures**.
  - Soft warnings: grounding, mesh connectivity, missing camera or light.
  - Plan-vs-state checks.
  — [SimWorlds](https://arxiv.org/html/2607.01766v1)
- **Budgets.**
  - Up to **10 coder retries and 5 tactical replans per stage**, then the stage aborts.
  - A hard verifier failure or a blocking review forces a retry.
  - If the final review fails, the scene rolls back to the last checkpoint, the coder gets one fix attempt, and the scene re-renders.
  — [SimWorlds](https://arxiv.org/html/2607.01766v1)
- **Ablation** (15 scenes). A visual-only verifier accepted results that were faked to look right. The authors stress that a correct-looking render doesn't prove the scene is built correctly. — [SimWorlds](https://arxiv.org/html/2607.01766v1)

  | Configuration | MPR | SPR | VLM score |
  |---|---|---|---|
  | Full | 0.97 | 0.97 | 0.87 |
  | No verifier | 0.93 | 0.87 | 0.84 |
  | No staged construction | 0.94 | 0.92 | 0.76 |

#### 3DCodeBench (arXiv 2606.01057, mid-2026; Blender 5.0) [PP, benchmark]
- **Setup.** 212 test instances; Blender 5.0; **240 s** timeout per script. Image metrics use **4 views at azimuth 45°/135°/225°/315°**. About 3,100 human votes were converted to Elo. — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
- **Model scores.** — [3DCodeBench](https://arxiv.org/html/2606.01057v1)

  | Model | Executability | SigLIP-2 | Elo |
  |---|---|---|---|
  | Claude Opus 4.7 | **0.910** | 0.814 | 1006 |
  | GPT-5.5 | 0.906 | 0.834 | **1163** |
  | GPT-5.4 | 0.866 | 0.817 | 1074 |
  | Claude Sonnet 4.6 | 0.804 | 0.813 | 1015 |
  | Gemini 3.1 Pro | 0.725 | 0.824 | 1147 |
  | Gemini 3.5 Flash | 0.464 | 0.824 | 1119 |

- **Error-feedback retry.** Up to 2 stateless retries, stopping at the first successful render.
  - Executability rose from **0.692 to 0.972** (table; the text says 0.702 to 0.974, +27.2 pp).
  - About **90% of baseline failures were Blender API mismatches**, e.g. `KeyError "Specular"` and `Mesh.use_auto_smooth`.
  - The stderr passed back was truncated to the first 70% plus the last 30%, within 33K characters.
  — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
- **Coding-agent harnesses** (Claude Code, Codex CLI, Gemini CLI). Executability rose to about 0.97–0.995, but conditional SigLIP-2 moved −0.010 and Uni3D −0.003. **Harnesses fix API errors, not shape quality.** — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
- **Thinking budget and views.** More thinking helps light models (Flash Lite gained about 19 executability points) but saturates for frontier models (under 5 points). Giving 1–4 reference views made almost no difference (SigLIP-2 varied by at most 0.012). — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
- **Visual self-critique.** Up to 2 iterations, tested on Gemini and Gemma models only. — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
  - Text-to-3D: mean SigLIP-2 change of +0.003 to +0.0085.
  - Image-to-3D: negative for 4 of 5 models (−0.0062 to −0.0088).
  - A **revert-on-break** guard restored the prior state when a "fix" failed to render. Smaller models reverted 26–28% of the time.
- **Remaining failures.** Even scripts that run often produce **disconnected or floating components**. Physical plausibility is the main bottleneck once code executes. Opus 4.7 qualitatively produced simplistic, floating primitives. — [3DCodeBench](https://arxiv.org/html/2606.01057v1)

#### BlenderRAG (arXiv 2605.00632, May 2026) [PP]
- **Method.** Retrieves the **3 most similar text→code examples** from 500 (50 categories × 10 variations; Nomic embeddings in Qdrant) and puts them in the prompt.
- **Results** on 30 out-of-distribution prompts:
  - Average compile success **40.8% → 70.0%**: Claude Sonnet 4.5 43.3→76.7; Gemini 3 Flash 53.3→80.0; GPT-5 Chat 56.6→66.7; Mistral Large 10.1→56.7.
  - CLIP alignment 0.409 → 0.774.
- **Failure modes.** Baselines showed floating components and inconsistent scales.
- **Caveat.** The evaluation is small.
- — [BlenderRAG](https://arxiv.org/html/2605.00632)

### Inferences
- **Build in gated stages, not one shot.** Staging (SimWorlds), subtask creation (LL3M), part-by-part assembly (L3GO) and the HN "ordered steps" workaround (Q2) all point the same way. Use reference → blockout → major parts → secondary detail → materials → polish/export. Don't advance a stage until its deterministic checks pass and its yes/no visual checklist is all "yes".
- **Look up docs before coding on Blender 5.2.2.** Every benchmark used Blender 4.4–5.1, and about 90% of failures were API drift. Call `search_api_docs` / `get_python_api_docs` before using any operator, node or property not already used successfully in this session, and immediately after any `AttributeError`, `KeyError` or `TypeError`.
- **Retry budget.** Give a failing code cell 2 retries with a truncated traceback. On the third failure, look up the docs and rewrite the smallest failing function instead of retrying again (3DCodeBench, LL3M).
- **Don't expect retries or the harness to improve shape.** Claude Code fixes runtime errors, but shape quality comes from decomposition, references, component functions and verification (3DCodeBench harness result).
- **Refinement budget.** Allow 2–4 visual rounds per stage. Snapshot the best version and revert if a round isn't an improvement (BlenderAlchemy peaked at edit 3, and dropping revert degraded results; 3DCodeBench used revert-on-break). With a small budget, generate 2–4 alternative variants of a part and pick by pairwise comparison. Run each comparison twice with the order swapped, to counter position bias. Shift effort to verification only when the budget is large (BlenderGym Table 2).
- **Order of trust for verification.** Deterministic engine-state checks are the hard gate (SimWorlds). Next come yes/no questions answered from renders (CADCodeVerify), then per-critique "was it applied?" re-checks (LL3M). Holistic VLM opinion comes last: it agrees with humans only 0.66 of the time.
- **Edit, never regenerate.** Each asset has one generator script as its source of truth. Keep it in context and patch functions (LL3M context sharing). Keep each editable unit under about 80 lines (BlenderGym).
- **Keep a project component library** (SceneCraft library learning, BlenderRAG examples, MeshCoder part APIs). Functions such as `hull_section()`, `engine_nozzle()`, `greeble_strip()`, `canopy()`, `fin()`, `emissive_trim()` and `corridor_module()` should each be tested once on 5.2.2. Then reuse them across hero ships, enemies and the mothership.
- **Claude-specific risk.** In 3DCodeBench, Claude's code ran most reliably but its shapes were less preferred by humans (simplistic, floating primitives). Skills for Claude agents should explicitly require shape-enriching steps (bevels, insets, panel cuts, curved profiles) and attachment checks. Opus 5.5 was not benchmarked.
- **Time expectations.** LL3M took about 10 min per object; SimWorlds took 70–120 min per scene. For a 4-day jam, budget roughly 30–90 min of agent time per hero asset and 10–30 min per prop or variant *(proposed)*.

### Gaps
- **Unretrieved numbers.** BlenderLLM's exact syntax-error and CADBench numbers. 3D-GPT's exact function library (likely Infinigen, unconfirmed in fetched text). ShapeCraft's quantitative results. MeshCoder's IoU figures, which come from a secondary summary.
- **Missing ablations.** No paper reports a controlled curve of quality vs number of critique rounds beyond 4; VIGA has no round analysis, and 3DCodeBench caps at 2. LL3M has no quantitative critic or verifier ablation. 3DCodeBench's self-critique results cover Gemini and Gemma only, not Claude or GPT, and its API-doc-retrieval scenario reported no numbers.
- **Not read.** EZBlender (arXiv 2601.07143, a Plan-and-ReAct editing agent) and the "Agentic Artifact Creation" survey (arXiv 2608.28122).
- **Peer review.** Not confirmed for LL3M, VIGA, SimWorlds, 3DCodeBench or BlenderRAG.
- **Blender 5.2.** No source documents 5.1→5.2-specific API changes.

---

## Q2. What practical workflows do experienced Blender MCP users (Claude, Cursor, Codex) report for production-quality rather than toy results?

### Takeaway
Practitioners converge on one loop:
1. Inspect the scene.
2. Write a brief with hard constraints.
3. Do a scoped blockout.
4. Pass a multi-view gate.
5. Add detail, then materials.
6. Do game-ready prep.
7. Export one asset early and test it in the engine.

They save versions before every batch operation and put a human or explicit gate at each stage. Screenshots are used to understand the scene; measured values decide acceptance. Hard-surface props and rooms are where MCP agents do well. Characters and organic forms still need other tools.

### Cited Findings

#### Tooling context (official server vs community server)
- **Official Blender Lab MCP server.** [BLOG] — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)
  - Releases: v1.0.0 on 27 Apr 2026, v1.0.2 on 8 Sep, v1.0.3 on 11 Sep.
  - Requires Blender 5.1+. GPL-3.0-or-later.
  - 26 tools, six of them `_for_cli` variants that open a .blend in a background process.
  - Runs model-written code **without guards**; Blender Lab recommends a VM or a system without sensitive data.
  - Anthropic's Claude Blender connector uses this server.
  - This session's tool list matches: 26 tools including six `_for_cli` variants, plus `jump_to_view3d_object_by_name` and `get_screenshot_of_window_as_json`.
- **Source conflict on minimum Blender version.** Other write-ups give Blender 4.2+ for the official connector ([JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)) or 3.0+ with 4.5 LTS recommended ([mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)). These likely conflate it with the community server. It doesn't matter for 5.2.2.
- **Anthropic announcement** (28 Apr 2026, nine creative connectors). [BLOG/news] — [9to5Mac](https://9to5mac.com/2026/04/28/anthropic-releases-9-new-claude-connectors-for-creative-tools-including-blender-and-adobe/); [OpenSourceForU](https://www.opensourceforu.com/2026/05/anthropic-strengthens-open-source-play-with-blender-backing-and-claude-connectors/)
  - Blender developers built the connector on MCP, so other LLMs can use it.
  - Anthropic's examples: analyze and debug whole scenes, write scripts that batch-apply changes, and add new tools to Blender's UI.
  - Anthropic joined the Blender Development Fund as a patron.
- **The server's own usage instructions** (delivered to the agent at connection; project at projects.blender.org/lab/blender_mcp). — [Blender Lab MCP project](https://projects.blender.org/lab/blender_mcp)
  - Treat `execute_blender_code` as a last resort when a dedicated tool exists.
  - Prefer `bpy.ops` for standard actions and `bpy.data` for precise control.
  - Set mode, active object and selection explicitly before operators, because operators change them as a side effect.
  - Update the depsgraph before reading computed values.
  - In Edit Mode use bmesh, and flush changes back.
  - Return structured data rather than prints.
  - Check scene units (meters by default).
  - Inspect large scenes progressively rather than dumping them.
- **Community server** (MCP for Blender, formerly blender-mcp; Siddharth Ahuja, Mar 2025; renamed 16 Sep 2026). [BLOG] — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)
  - Blender 3.0+; 36 tools.
  - Integrations: Poly Haven, Sketchfab, Poly Pizza, Hyper3D Rodin, Hunyuan3D, Tripo.
  - Runs arbitrary Python by default (safe mode is opt-in) over an unauthenticated socket.
  - The official connector has none of the text-to-3D or asset-library integrations ([mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)).

#### Reported workflow recipes
- **Game-asset pipeline** (best-games.io, 20 Jul 2026). [BLOG] — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
  1. Asset brief: grid, pieces, style, budget, origin, delivery format.
  2. Blockout with boxes, cylinders, bevels and simple modifiers only.
  3. **Four-view check**: front, side, back and game camera (the example prompt adds top).
  4. Structure, UVs and materials, only after the blockout passes.
  5. Game-ready prep: apply transforms, check normals, remove duplicates, verify UVs, set origins, add collision, make LODs.
  6. Export and import **one representative module early**.
  7. Player test with normal controls.
  8. Revise.
- **Human gates** (same source). Stop after the blockout passes the four-view check, after the first module round-trips through the engine, and after validation. The agent shouldn't be the final gate for hero characters, topology, UVs, rigging, art consistency, performance or licensing. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **Official-connector guide** (mcp.directory, 29 Apr 2026). [BLOG] — [mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)
  - Inspect first: summarize collections, object counts, missing materials and broken modifiers before changing anything.
  - Scope batch work to a named collection.
  - Confirm before destructive steps.
  - Chunk large jobs one collection at a time.
  - Verify with viewport screenshots over several turns.
  - Once a workflow works, wrap it as a permanent operator.
  - Only one MCP client can connect to a Blender session at a time.
- **StraySpark recommendations** (25 Sep 2026, updated 3 Nov 2026). [BLOG] — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)
  - Inspect the scene, propose changes, then report which requirements can be verified.
  - Look up the API in the bundled docs or by live introspection instead of guessing.
  - **Treat screenshots and summaries as aids to understanding, and use measured values for checks.**
  - Use the `_for_cli` variants for batch audits of files that aren't open.
  - **Re-import exports into a fresh scene** and verify them.
- **Claude Opus 5.5 + Blender MCP creature test** (JQ Visions, 21 Oct 2026; experiment by Aidan Stanik). [BLOG/ANEC] — [JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)
  - Started from a reference image and a conversational brief.
  - Produced a 1.4M-polygon environment and a 115-bone rig.
  - Problems: a finger intersected a branch and needed a manual fix, a black-material glitch appeared halfway through the final render, and the caterpillar looked "toy-like".
  - The post recommends contact sheets at each approval gate; solid, wireframe and rendered views; low-res beauty passes; object-ID checks; and first/middle/last frame checks before the final render.
  - Briefs should state resolution, polygon budget, approved asset sources, license rules and review gates.
  - Verdict: promising but assisted.
- **What works and what doesn't** (MindStudio, 6 May 2026). [BLOG] — [MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)
  - Works well: scene setup, hard-surface and geometric scenes, basic Principled BSDF materials, duplicating and arranging objects, utility scripts.
  - Works poorly: organic shapes, characters, faces, foliage; clean topology; complex modifier stacks; multi-object alignment; procedural textures; Geometry Nodes ("brittle"); visual judgment of proportions; precise placement, which often needs several correction rounds.
  - Other failures: Blender crashes break the connection, and scripts break across API versions.

#### Prompt patterns reported
- **Scoped blockout prompt.** Give the module list, naming prefix, origin rule and grid. Explicitly exclude textures, decorations, particles and characters from the first pass. Ask for **dimensions and triangle counts per object plus named screenshots**, ask the agent to look for specific defects (gaps, overlaps, misalignment), and have it save a new file before corrections. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **First connection test.** Create one cube at a stated size, move it to an exact position, color it, take a screenshot, and save a new file. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **Diagnostic prompt.** For a black render: check render settings, world, lights, camera and view-layer visibility, then propose the most likely cause before changing anything. — [mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)

#### Screenshot and render feedback specifics
- **Screenshot budget.** 3–4 screenshots per turn is fine; about 30 per session noticeably raises token cost. — [mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)
- **Example final-render tiers.** 480p at 64 samples, 1080p at 256, 4K at 1024. — [mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)
- **Check other angles.** Never approve an asset only from the angle used in the prompt. Thin backs, floating parts, reversed normals and intersections often show only from other angles. Ask the agent to list what changed between revisions. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **Research camera rigs.** — [LL3M](https://arxiv.org/html/2508.08228v1); [3DCodeBench](https://arxiv.org/html/2606.01057v1); [VIGA](https://arxiv.org/html/2601.11109v2); [BlenderGym](https://arxiv.org/html/2504.01786v1)
  - LL3M: 5 views, distance fitted to the bounding box, Eevee.
  - 3DCodeBench: 4 views at 45°/135°/225°/315° azimuth.
  - VIGA: cameras at the 4 corners of the bounding box.
  - BlenderGym: at least 3 fixed views, including one elevated overview.

#### Asset sources and when to switch tools
- **Licensing.** Poly Haven assets are CC0. Sketchfab licenses vary, and some restrict commercial game use. Keep an asset manifest (source, creator, license, date, edits). For AI-generated models, record service terms, prompts, reference images and the generation date, and give them the full production review. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **Poly Pizza.** About 10,600 free low-poly models; filter to CC0 to keep the style and license consistent. [BLOG] — [MCP for Blender gaming use case](https://mcp-for-blender.com/use-cases/gaming)
- **When to switch.** Use prompt-to-3D tools (Meshy, TripoSR, Shap-E) for single organic meshes, BlenderKit or other libraries to avoid modeling from scratch, and manual modeling for production quality or tight deadlines. — [MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)

#### Community anecdotes
- **Hacker News** (2025 threads) [ANEC]. Commenters say Blender MCP only partly works and is weak at understanding 3D space. One user found that agent-written Python tops out at simple primitives; their workaround was to split the model into ordered part-steps run one at a time. The quote-to-thread attribution comes from a search summary. — [HN Show HN thread](https://news.ycombinator.com/item?id=44622374)
- **Community "blender-modeler" skill for Claude** [ANEC; no evidence it was tested]. — [arjun988/blender-skills](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md)
  - Workflow: scene setup → blockout → refinement → modifier stack → cleanup → handoff.
  - Collections `COL_Reference/Blockout/Geo/Collision/Instances`; `SM_` name prefix.
  - Modifier order (bottom to top): **Mirror → Array → Solidify → Boolean → Bevel → Weighted Normal → Subdivision**.
  - Keep booleans live until export, and apply scale before booleans.
  - Instance repeated parts: collection instances for modular kits, linked duplicates, arrays.
  - Query the polycount after each major edit.
  - Its "Auto Smooth 30–60°" advice is outdated: `Mesh.use_auto_smooth` no longer exists in Blender 5.x and is one of the most common drift errors ([3DCodeBench](https://arxiv.org/html/2606.01057v1)).
- **Other community hard-surface skills** surfaced in search [ANEC; which file says what is unverified]. Advice: put Bevel after Boolean, and instance small detail meshes such as bolts instead of modeling each one. One skill advises against weighted normals on hard surfaces, contradicting the others. — [composio opencode-skills](https://github.com/composio-community/opencode-skills/blob/master/skills/blender-hard-surface-modeling/SKILL.md); [skillsdirectory listing](https://www.skillsdirectory.com/skills/scenario-labs-scenario-blender-hard-surface)

### Inferences
**Session protocol for Triad Drift, mapped to the official server's tools:**
1. **Safety and versions.** Work on a copy, and save a numbered .blend before every batch or destructive step. The server has no guards, so keep sensitive data off the machine or use a VM.
2. **Inspect.** Run `get_objects_summary`, then `get_object_detail_summary` on relevant objects. Read the unit settings. Never assume names exist.
3. **Brief, per asset.** State target dimensions, forward axis, origin rule, name prefix and collection, triangle budget, material-slot budget, the reference or concept image, and what the first pass excludes.
4. **Blockout.** Place a scale reference (player capsule or cockpit-sized box). Use primitives and the mirror modifier only. Report dimensions and triangles per object. Render a fixed camera rig plus the game camera with `render_viewport_to_path` or `render_thumbnail_to_path`. Gate before continuing.
5. **Major parts.** One function per part in the asset's generator script. After each part: run checks, then take one cheap `get_screenshot_of_area_as_image` (use `jump_to_view3d_object_by_name` to frame a close-up).
6. **Secondary detail.** Bevel and weighted-normal modifiers, panel insets and cuts, instanced greebles.
7. **Materials.** Few slots, bold saturated base colors, and emission for engines and trims. Avoid deep node graphs, since node wiring is the main source of non-executable material code (BlenderGym; MindStudio).
8. **Game-ready prep and early export.** Apply transforms, fix normals, set origins, add collision and LODs. Export one representative asset to UE5 on day 1. Re-import the exported file in a background session (`execute_blender_code_for_cli`) and audit it.
9. **Promote what works** into the shared component library or an operator, so the next asset starts from tested code.

**Prompt shape for each stage:** goal → constraints in numbers → what not to touch → which checks to run and report (dimensions, triangles, names) → which views to render → "stop and report; don't continue to the next stage".

### Gaps
- Reddit (r/blender, r/ClaudeAI, r/3Dmodeling) could not be fetched; the fetch tool blocks reddit.com. X threads and YouTube transcripts weren't reachable either, so community evidence is limited to Hacker News and blogs.
- No first-party OpenAI (Codex) or Cursor posts about 3D modeling with Blender MCP were found.
- No practitioner write-up of a code-built spaceship or sci-fi hard-surface asset with before/after quality evidence was found.
- No practitioner reports iteration counts.
- The practitioner sources give no concrete UE5 export settings (scale, axes, smoothing groups); best-games says only that Unreal's FBX static-mesh pipeline supports materials, custom collision, multiple UV sets and LODs.
- What changed between Blender Lab MCP v1.0.1 (ours) and v1.0.2/v1.0.3 is unknown.

---

## Q3. What checklists and automated checks do agents use to self-verify?

### Takeaway
The best-supported design has two tiers.
- **Hard gates:** deterministic engine-state checks — naming and hierarchy, dimensions against the brief, contact and interpenetration via BVH, connectivity and manifold, normals, triangle counts, and an export round-trip. Removing SimWorlds' verifier cut structural pass rates from 0.97 to 0.87.
- **Perceptual criteria:** renders from a fixed multi-view rig, judged against **specific yes/no questions** rather than holistic opinion, with a revertable best version always kept.

Silhouette-only renders and numeric triangle budgets are not documented in the agent literature found.

### Cited Findings

#### Deterministic checks
- **SimWorlds verifier**, run in the orchestrator after every stage. Removing it cut SPR from 0.97 to 0.87. — [SimWorlds](https://arxiv.org/html/2607.01766v1)
  - Structural: collection tiers, a root Empty per object, parent chains, naming, a ground object.
  - BVH geometry: declared contacts within tolerance; undeclared interpenetrations fail hard.
  - Warnings: grounding, mesh connectivity, missing camera or light.
  - Plan-vs-state comparison.
- **Dimensions and scale.**
  - Create a reference object (a 2 m measuring object or player capsule).
  - Lock forward axis, world up, unit scale, grid increment and origin convention before detailed work.
  - Check dimensions against the brief (an arch measuring 1.7 m against a 2 m brief is an error).
  - Use a 2 m grid with an 8×8 m test room for modular kits; put origins at the bottom corner used for snapping, and test by placing several rotated copies.
  — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)
- **Triangle and material budgets.**
  - Report triangle counts per object; use one material per module and a 1024 texture atlas (no numeric triangle limits given) — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide).
  - Query the polycount after each major edit and compare it to the budget — [community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md).
- **Cleanup and manifold checklist** [ANEC]: merge by distance at 0.0001 m, remove loose geometry, delete interior faces, recalculate outside normals, apply scale where modifiers depend on it, verify the mesh is manifold, check triangles against budget. — [community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md)
- **Cleanup for imported or AI meshes** [BLOG]. — [Seele](https://www.seeles.ai/resources/blogs/ai-3d-model-cleanup-after-meshy-tripo-hunyuan)
  - Check non-manifold edges, holes, duplicate and internal faces, normals, UVs, **silhouette loss**, scale, pivots and collision.
  - **Repair broken geometry before reducing polygons.** After decimating, compare silhouette, normal shading and UVs.
  - Import the actual exported file into the target engine.
- **Export round-trip.** Re-import exports into a fresh scene and verify them. Use measured values, not screenshots, for checks. — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)

#### Visual and perceptual checks
- **Fixed multi-view rigs.** — [3DCodeBench](https://arxiv.org/html/2606.01057v1); [LL3M](https://arxiv.org/html/2508.08228v1); [VIGA](https://arxiv.org/html/2601.11109v2); [BlenderGym](https://arxiv.org/html/2504.01786v1); [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide); [JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)
  - 3DCodeBench: 4 azimuths, 45°/135°/225°/315°.
  - LL3M: 5 views, camera distance fitted to the bounding box, Eevee.
  - VIGA: cameras at the 4 corners of the bounding box, plus zoom and move to investigate.
  - BlenderGym: at least 3 fixed views, including an elevated overview, with every object visible in at least one.
  - best-games: front, side, back, top, plus game camera.
  - JQ Visions: solid, wireframe and rendered shading modes.
- **Yes/no question checklists.** Generate binary questions from the brief, answer each from renders, feed back only the failures, and stop when all pass. Few-shot example questions and reference images were both critical. — [CADCodeVerify](https://arxiv.org/abs/2410.05340v1)
- **Was each critique applied?** After every fix pass, a separate verifier re-renders and checks each critique item, because coders often apply only some fixes. — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Revert to best.**
  - Keep the current best in every comparison; removing revert degraded later iterations — [BlenderAlchemy](https://arxiv.org/html/2404.17672v1).
  - Restore the prior state when a fix breaks the render — [3DCodeBench](https://arxiv.org/html/2606.01057v1).
  - Roll back to a checkpoint plus one fix attempt — [SimWorlds](https://arxiv.org/html/2607.01766v1).
- **Judge reliability.** VLM pairwise judgment agrees with humans about 0.66 of the time (human–human 0.79). Some models show position bias, and more verifier compute improves selection. — [BlenderGym](https://arxiv.org/html/2504.01786v1)

#### Failure modes → evidence → mitigations that worked

| Failure mode | Where observed | Mitigation used or validated |
|---|---|---|
| Floating or disconnected parts | LL3M (handles, chair legs) [LL3M](https://arxiv.org/html/2508.08228v1); common even when code runs, "floating primitives" from Opus 4.7 [3DCodeBench](https://arxiv.org/html/2606.01057v1); RAG baselines [BlenderRAG](https://arxiv.org/html/2605.00632) | BVH contact check within tolerance, plus grounding and connectivity warnings ([SimWorlds](https://arxiv.org/html/2607.01766v1)); critic → verifier → user edits ([LL3M](https://arxiv.org/html/2508.08228v1)); multi-angle review ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) |
| Intersecting parts | Pass visual checks ([SimWorlds](https://arxiv.org/html/2607.01766v1)); finger through a branch, fixed manually ([JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)) | Undeclared BVH interpenetration is a hard fail ([SimWorlds](https://arxiv.org/html/2607.01766v1)); four views plus game camera ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) |
| Wrong scale or proportions | Inconsistent scales ([BlenderRAG](https://arxiv.org/html/2605.00632)); 1.7 m arch vs 2 m brief ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)); weak proportion judgment ([MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)) | Scale reference object and dimension report against the brief ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)); measured values over screenshots ([StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026)); critic asked to look for proportion errors ([LL3M](https://arxiv.org/html/2508.08228v1)) |
| Wrong orientation or pivot | Modular kits ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) | Lock forward axis, up, unit scale and origin before detail; origin at the snapping corner; test with rotated copies ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) |
| Blocky, primitive-only output | [HN](https://news.ycombinator.com/item?id=44622374); [3DCodeBench](https://arxiv.org/html/2606.01057v1); [MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance) | Doc retrieval gave 5× more complex operations: bmesh, curves, geometry nodes ([LL3M](https://arxiv.org/html/2508.08228v1)); retrieved code examples ([BlenderRAG](https://arxiv.org/html/2605.00632)); modifier stack ([community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md)); reference image ([BlenderAlchemy](https://arxiv.org/html/2404.17672v1)) |
| Flipped normals | Hidden from the prompt angle ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) | Recalculate outside normals in cleanup ([community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md)); normals check in prep; multi-angle review |
| Non-manifold geometry, holes, internal faces | [Seele](https://www.seeles.ai/resources/blogs/ai-3d-model-cleanup-after-meshy-tripo-hunyuan); [community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md) | Merge by distance 0.0001 m, remove loose, delete interior faces, verify manifold; repair before decimating |
| Flat or broken materials | Node-wired material scripts often fail ([BlenderGym](https://arxiv.org/html/2504.01786v1)); Principled BSDF OK, procedural textures poor ([MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)); black-material glitch mid-render ([JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)) | Tree search with visual imagination raised CLIP from 25.2 to 28.2 ([BlenderAlchemy](https://arxiv.org/html/2404.17672v1)); low-res beauty passes and first/middle/last frame checks ([JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)) |
| Losing track of objects and names | Regenerates a different asset without code context ([LL3M](https://arxiv.org/html/2508.08228v1)) | Shared code context ([LL3M](https://arxiv.org/html/2508.08228v1)); naming, parent-chain and collection checks ([SimWorlds](https://arxiv.org/html/2607.01766v1)); memory of code diffs and renders ([VIGA](https://arxiv.org/html/2601.11109v2)); prefixes and collections ([best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)) |
| API drift and runtime errors | About 90% of failures ([3DCodeBench](https://arxiv.org/html/2606.01057v1)); "Specular" renamed ([LL3M](https://arxiv.org/html/2508.08228v1)) | Doc retrieval (26% fewer errors); 2 error-feedback retries (executability 0.69→0.97) |
| Critique only partly applied | [LL3M](https://arxiv.org/html/2508.08228v1) | Verification agent re-checks each item |
| "Fixes" that make it worse | Image-conditioned critique was negative for 4 of 5 models ([3DCodeBench](https://arxiv.org/html/2606.01057v1)) | Revert-on-break; current best stays in the tournament ([BlenderAlchemy](https://arxiv.org/html/2404.17672v1)) |
| Looks right but is built wrong | Visual-only verifier accepted faked results ([SimWorlds](https://arxiv.org/html/2607.01766v1)) | Engine-state audit |
| Judge picks the wrong candidate | Position bias, 0.66 agreement with humans ([BlenderGym](https://arxiv.org/html/2504.01786v1)) | More verifier compute; deterministic checks first |

#### Blender API hooks for implementing these checks
Standard API references, not fetched this session; confirm signatures with `search_api_docs` on 5.2.2.
- `mathutils.bvhtree.BVHTree.FromObject(...)`, `.overlap(other)` and `.find_nearest(...)`, for interpenetration and contact distance — [Blender API: mathutils.bvhtree](https://docs.blender.org/api/current/mathutils.bvhtree.html)
- `BMEdge.is_manifold` / `is_boundary` and `BMVert.is_manifold`, for non-manifold counts — [Blender API: bmesh.types](https://docs.blender.org/api/current/bmesh.types.html)
- `bmesh.ops.remove_doubles` and `bmesh.ops.recalc_face_normals`, for cleanup — [Blender API: bmesh.ops](https://docs.blender.org/api/current/bmesh.ops.html)
- `Object.dimensions`, `Object.bound_box` and `matrix_world`, for dimension and bounding-box checks — [Blender API: Object](https://docs.blender.org/api/current/bpy.types.Object.html)

### Inferences
**Proposed `audit_asset(collection)` check suite**, returning a dict. Thresholds are *(proposed)*.

**Hard gates:**
1. **Names and hierarchy.** Every object has the asset prefix, sits in the asset's collection, and has an expected parent. No `.001`-style duplicate suffixes. All expected part names from the generator's registry exist.
2. **Dimensions.** Overall bounding box within ±5% of the brief on each axis *(proposed)*. Forward axis and up axis as agreed. Origin at the agreed point. Scale applied (1, 1, 1).
3. **Attachment.** For each part, the BVH nearest distance to its parent or the hull is ≤ 1 mm *(proposed)*. Otherwise flag it as floating.
4. **Interpenetration.** Report overlap pairs between parts unless the part is tagged `embedded` (kitbash greebles intentionally sink into the hull).
5. **Mesh health.** Non-manifold edge count 0 for parts meant to be closed. No loose vertices. No zero-area faces. Normals consistent (count faces whose normal flips after `recalc_face_normals`).
6. **Budgets.** Triangles per object and in total, against the asset budget. Material slots ≤ 3 per prop and ≤ 5 per hero ship *(proposed)*.
7. **Export round-trip.** Export, re-import in a background session, and re-run checks 1–6.

**Perceptual tier, after the hard gates pass:**
- **Render rig.** 4 azimuth views (45/135/225/315) at about 25° elevation; orthographic front, side and top; plus the game camera (a chase camera behind and above for ships). Combine into one contact sheet to stay within 3–4 images per turn.
- **Questions.** Have the VLM answer 5–10 yes/no questions written from the brief, for example: "Is the silhouette readable from the top?", "Are all three engine glows visible from the chase camera?", "Is there any part not touching the hull?".

**Silhouette test** *(proposed, untested)*: render flat black on white (Workbench, flat lighting, object color) at gameplay-like size (about 128 px) from top, side and chase camera. Ask whether the three hero ships are distinguishable from each other by silhouette alone.

**Stop rule** *(proposed)*: advance a stage when all hard gates pass and all yes/no items are "yes", or after 3 visual rounds, taking the best snapshot and logging the remaining issues for a human.

### Gaps
- No agent paper or practitioner source documents silhouette-only tests. The closest is Seele's "silhouette loss" check after decimation.
- No numeric triangle or texture budgets for UE5 jam assets were found within this scope; best-games gives none.
- SimWorlds doesn't state its contact tolerance value.
- No source compares different camera-rig sizes for verification quality. BlenderGym didn't ablate the view count, and 3DCodeBench's 1–4 view test concerns generation inputs, not verification.

---

## Q4. Where are the limits, and which hybrid strategies are recommended instead?

### Takeaway
Code-driven agents are dependable for hard-surface, architectural/modular and simple procedural assets. They are weak at:
- organic characters, faces and foliage;
- clean, deformable topology and sculpted detail;
- precise multi-object spatial placement;
- aesthetic judgment;
- complex node graphs.

Even on easy edits, the best VLM systems are far below average Blender users. Recommended hybrids: image-to-3D generation followed by cleanup and retopology; CC0 asset libraries; kitbashing from a component library with instancing; and AI concept images as references. Keep code for assembly, cleanup and checks.

### Cited Findings
#### Limits
- **Weak areas.** Organic shapes, characters, faces, realistic trees and foliage; clean edge flow for subdivision or animation; complex modifier stacks; multi-object alignment; procedural textures; Geometry Nodes ("brittle"); rigging and IK; judging proportions; precise placement. [BLOG] — [MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)
- **Organic topology and animation.** Production-ready organic character topology and nuanced facial animation remain weak and need specialist review. A Claude-built caterpillar looked toy-like. [BLOG/ANEC] — [JQ Visions](https://visions.joaoqueiros.com/blog/claude-opus-5-5-blender-mcp-3d-modeling-rigging-animation-workflow/)
- **Human gap.** The best VLMs trail average Blender users by roughly 5–25× in photometric loss on geometry, placement and blend-shape edits. [PR] — [BlenderGym](https://arxiv.org/html/2504.01786v1)
- **Physical plausibility** is the main bottleneck once code executes; floating or disconnected components are common. [PP] — [3DCodeBench](https://arxiv.org/html/2606.01057v1)
- **Spatial errors persist.** VLM critics miss some spatial artifacts, and spatially specific edits take several user prompts. [PP] — [LL3M](https://arxiv.org/html/2508.08228v1)
- **Primitives ceiling.** Agent-written Python tops out at simple primitives unless the work is split into ordered steps. [ANEC] — [HN](https://news.ycombinator.com/item?id=44622374)
- **Broader agentic game dev.** In GameCraft-Bench the strongest agent reached 41.46% on Godot tasks. [2°, cited by a blog] — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide)

#### Hybrid strategies
- **Image-to-3D, then cleanup.**
  - Raw Hunyuan3D outputs can exceed 500,000 triangles and need retopology before game use. [BLOG] — [Zentor](https://zentor.ai/blog/hunyuan-3d-agent-tool)
  - In one 2025 Blender test, Hunyuan3D gave the cleanest geometry, while Meshy and Tripo needed moderate cleanup. [BLOG][OLD-ish] — [Top3D](https://www.top3d.ai/learn/retopology-comparison-2025)
  - Typical artifacts such as a stray ground plane take about a minute to remove. Tripo exports GLB that opens in Blender, Unity or Unreal. [BLOG] — [Krea](https://www.krea.ai/blog/best-ai-3d-model-generators-2026)
  - Pipeline several sources describe: clean the reference image → generate multiple views → image-to-3D → retopology to a face budget → UVs → texture → export FBX or glTF. Most sources are vendor content. — [Krea](https://www.krea.ai/blog/best-ai-3d-model-generators-2026); [Medium review](https://medium.com/ideas-with-wings/best-image-to-3d-tools-7eea7b05eb11)
  - Fix geometry before decimating, then compare silhouette and shading after. — [Seele](https://www.seeles.ai/resources/blogs/ai-3d-model-cleanup-after-meshy-tripo-hunyuan)
- **Generator access.** The community MCP server integrates Hyper3D Rodin, Hunyuan3D, Tripo, Poly Haven, Sketchfab and Poly Pizza. The official server and connector do not, so those assets have to be generated elsewhere and imported. — [StraySpark](https://www.strayspark.studio/blog/official-blender-mcp-server-comparison-2026); [mcp.directory](https://mcp.directory/blog/claude-blender-connector-guide)
- **Asset libraries.** Poly Haven (CC0); Sketchfab (mixed licenses); Poly Pizza (about 10.6k low-poly models, CC0 filter); BlenderKit. Keep an asset license manifest. — [best-games.io](https://best-games.io/blog/codex-claude-code-blender-mcp-game-development-guide); [MCP for Blender](https://mcp-for-blender.com/use-cases/gaming); [MindStudio](https://www.mindstudio.ai/blog/claude-blender-mcp-real-world-performance)
- **Retrieval inside research loops.** SceneCraft retrieves the top-10 CLIP-ranked assets, renders each and keeps the best ([SceneCraft](https://arxiv.org/html/2403.01248v1)). VIGA's generator has a `get_better_assets` tool ([VIGA](https://arxiv.org/html/2601.11109v2)).
- **AI concept images as references.** Generating a target image from text ("visual imagination") improved material edits; removing it flattened scores ([BlenderAlchemy](https://arxiv.org/html/2404.17672v1)). Reference images were critical to CADCodeVerify's verification ([CADCodeVerify](https://arxiv.org/abs/2410.05340v1)). Critique that tries to match a reference image regressed 4 of 5 models ([3DCodeBench](https://arxiv.org/html/2606.01057v1)).
- **Kitbashing and instancing.** Collection instances for modular kits, linked duplicates for props with variants, Geometry Nodes for scatter, arrays for linear repetition; instance small details instead of modeling each one. [ANEC] — [community skill](https://github.com/arjun988/blender-skills/blob/main/.claude/skills/blender-modeler/SKILL.md)
- **Mesh → editable code (research only).** MeshCoder turns point clouds into part-structured Blender Python, so a generated or scanned mesh could become editable code. It is a research model, not a jam-ready tool. — [MeshCoder](https://arxiv.org/html/2508.14879v1)

### Inferences
**Per-asset routing for Triad Drift** *(proposed; based on the strengths and limits above)*:

| Asset | Route | Notes |
|---|---|---|
| 3 hero ships | **Code-first** parametric generator script per ship, built from a shared component library | Mirror, bevel and weighted normal; instanced greebles; emissive engine nozzles and trims; one AI concept sheet per ship (front/side/top) as reference; silhouette test to keep the three ships distinct |
| Enemy ships | **Code**: parameter variants of the hero components | Swap and scale parts, change palette (SceneCraft/LL3M-style parametric reuse) |
| Giant mothership | **Code kitbash**: collection instances and arrays of the ship components at large scale | Check scale against the hero ships; mind triangle totals |
| Asteroids, rocks | **Code procedural** (displaced icospheres, random seeds) or Poly Haven CC0 rocks | Low risk |
| Crystals | **Code** (scaled prisms or convex hulls) with emissive material | Low risk |
| Alien flora | **Hybrid**: simple curve-based or array-based code; fall back to image-to-3D plus cleanup for hero plants | Foliage and organic shapes are a known weak spot; keep Geometry Nodes shallow |
| Modular corridor kit | **Code-first** on a fixed grid, origin at the snapping corner | Export one module to UE5 on day 1 (best-games) |
| Props | **Code** or Poly Pizza / Poly Haven CC0 | Restyle library assets with the project palette |
| Energy blade | **Code**: simple blade mesh plus emissive core | Glow comes mostly from the UE5 material and bloom (not verified here) |
| Humanoid crew enemies | **Don't code-model.** Use image-to-3D (Hunyuan3D cleanest per one 2025 test) plus decimate/retopology and checks, or a CC0 library character | Alternatively, design the crew as armored or robotic hard-surface figures so code can build them. Rigging and animation needs a separate plan (out of scope) |

- Whatever the route, code stays responsible for **assembly, cleanup, naming, scale normalization, the audit suite and export**. That matches what agents do reliably: utility scripts, batch operations and measured checks.

### Gaps
- No controlled study compares code-modeling against image-to-3D on hard-surface game assets. Image-to-3D evidence is mostly vendor or affiliate blogs, plus one 2025 test.
- No source covers auto-rigging humanoid crew for UE5 within this research scope.
- No source quantifies how far Geometry Nodes generated by agents can be pushed (only "brittle" [BLOG]). LL3M shows geometry-node setups working qualitatively.
- No source covers moving Blender procedural shader materials into UE5. Inference to verify with the export-focused notes: Blender node materials generally don't carry over through FBX and must be baked or rebuilt in UE5.
