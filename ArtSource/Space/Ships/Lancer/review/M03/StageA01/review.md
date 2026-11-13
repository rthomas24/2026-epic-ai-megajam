# Lancer M03 structural A01 — failed assembly gate

Actual Blender5.1.1 source: production/M03/Lancer_M03_structure_v01.blend.
17,104 assembled triangles,12parts,8unique meshes,6materials;803.999996x719.999981x202.099997cm (UE L/W/H).
Geometry errors0;36frame checks and12muzzle/exhaust rays pass; canonical33frame byte guard pass.
Assembly FAIL: hidden enclosed220tri NoseRecess island, centerUE(391.5,0,0)cm,5.85cm gap. No exterior engine float demonstrated.
Warnings: two unusedComposite wing slots;38coincident/back-to-back saddle/barrel pairs per side;groundZ-0.96m (flight asset root, not ground prop).
Actual process4076 exit0,11.210s,peak798.77MiB,minfree4.468GiB,two threads. stdout/stderr retained; no retry in this slot.
Images are actual hero+six-view clay sheet; individual clay views were removed by the existing helper keep_views=False setting, as recorded in clay_manifest.json.

Fresh independent reviewer /root/lancer_m03_structure_review scored silhouette/proportion/construction/edges3/5 each; incomplete material/detail categories were not scored as final.
Three MAJOR fixes: broader flattened armored prow (~45–60cm terminal, not a tube); recessed rocket exits in integrated belly fairings rather than hanging boxes/exposed tubes; continuous intake-to-engine backbone plus root transition.
Reviewer dimensions are modeling proposals, not measured image dimensions. BLOCKER: remove/attach enclosed nose insert before clean checkpoint. Not final hero approval.

Prepared targeted corrections (not yet Blender-verified): widen forward-middle prow and50x24cm keel terminal; actual rectangular/chamfered prow bore and integral rim; remove hidden insert; raise/taper bay shell into wing volume with four real recess bores around unchanged exits; connect shoulder to engine case; extend cannon saddle2cm into collar to avoid coincident caps; remove unused slots without changing assignments.
Legacy lower rocket exits remain83cm below root, constraining lower fairing depth; never claim their frames moved. No native/collision/firing change.
Existing A01source and all14generated diagnostic files are preserved. Future structural saves use an incremented filename; reports/images archived here are not final Review01.
