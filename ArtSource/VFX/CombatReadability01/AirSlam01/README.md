# AirSlam01 — source implementation and motion preview

One on-foot landing cue, requested by Nicholas and coordinated through N-040.
Mainc4b6b029 publishes this exact source claim. Ryan's cue/binding/native answer
is pending. This is a SOURCE PREVIEW, not a Niagara asset or game integration.
Hero stays paused. No Combat/Verdance preset, shared map, Source/config, existing
VFX, presentation data asset, Sound or Shake field has been changed.

The effect expresses downward weight through outward ground compression:
one thin fractured ring accelerates to a300cm asymptote, followed by a shallow
dust skirt, eight rotating ballistic fragments and ten short contact glints.
The cyan energy front fades within420ms; dust disappears within820ms. The full
review timeline is1.15s. No full-screen flash, light, camera shake or large glow
disk. Noise and sector gaps interrupt uniform perfect-circle geometry.

slam.json is the authoring contract. The two .ush files implement the ring and
low-opacity dust material kernels as UE Custom expressions. They are source
only; standalone HLSL checks cannot establish Unreal material/Niagara validity.
The native system still needs emitters, mesh/sprite renderers, surface alignment,
compile/reload/capture/performance verification and the owner's slot binding.
The proposed ceiling is35 particles across four emitters, not a measured cost.
One ring's trailing band is shader detail, not an extra simulated particle.

The interactive preview is at the thread-owned visualization directory as
air-slam-preview.html. It implements timing and deterministic ballistic/drag
motion with a projected character for scale; three soft lobes approximate each
dust particle. It is not Unreal, a material screenshot or a GPU-cost simulation.
The preview's theme-aware colors approximate the intended native palette.

Current landing hook: UTDEnergyBladeComponent::NotifyOwnerLanded calls
PlayBladeCue(AirSlam,...,false,Owner->GetActorLocation()). That is CAPSULE CENTER.
Standing half-height90cm and crouched50cm are current source references; a
fixed-90cm system offset is unsafe. Ryan must confirm a real floor contact and
surface normal or an equivalent correct spawn transform, including slope and
crouch cases. Do not bind a ground ring at the current center location and call
it finished. Preserve current damage radius/timing and avoid implying the
visual ring is a precise projected damage boundary.

Next gate after owner reply: isolated native compile and fresh reload, then
actual standing/crouched/sloped landing and near-wall playtest with the current
on-foot camera; no occluded enemy silhouettes or damage telegraphs, no repeated
loop/retrigger and no world-space particles following the player after landing.
Profile at gameplay distance and compare with effects disabled. Nicholas reviews
the resulting real-game capture before significant look changes are integrated.
