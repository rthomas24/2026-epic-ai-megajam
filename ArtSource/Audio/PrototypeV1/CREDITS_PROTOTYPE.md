# Audio Prototype V1 — source and licensing handoff

Nick authorized prototype implementation of the existing audition on November 12, 2026. This is prototype permission, not final in-game audio approval.

| Used source pack | Publisher / exact assets | Rights and restrictions checked November 12, 2026 |
|---|---|---|
| [50 Free Game Sounds Pack](https://www.fab.com/listings/b8cc7270-5e0c-4ef7-a277-6a1d4b69358d) | PlaceHolder Inc: `Sci-Fi_Gun_1-1`, `Fire_Whoosh_2-15`, `Explosion_Medium_2-1`, `Shield_Metal_Impact_1-3`, `Hit_Generic_2-1`, `Whoosh_4-1` | Owned/cache accessible; publisher describes original human-made game sounds. Treat as licensed content, not public domain. Fab commercial-project terms support incorporation; exact acquisition license is not exposed by the local entitlement metadata. Native/raw Git publication remains held. |
| [Terminal User Interface Sound Effects Pack LITE Edition](https://www.fab.com/listings/183a6ec7-cbc3-451a-8fa7-08da97c07fcf) | Hove Audio: `CMPTMisc_ComputerAmbienceLoop_HA_TerminalUI_06_Wav`, `MACHAppl_PowerUp_HA_TerminalUI_02_Wav`, `CMPTMisc_DiskTrayClose_HA_TerminalUI_02_Wav`, `MACHAppl_VCRButton_HA_TerminalUI_04_Wav`, `CMPTMisc_PowerButton_HA_TerminalUI_02_Wav` | Official listing free; recorded hardware subset chosen, retro synthesized bleeps omitted. Same acquisition-license verification gate. |
| [Atmoshpheric Worlds — FREE Game Music Pack — No AI](https://www.fab.com/listings/c9e03a63-9359-4929-9c8a-a680309dbd3f) | Marika Schanz: `Nordic_Exploration__The_Ancient_Moon`, `Trailer_Solaric` | Publisher explicitly permits commercial and non-commercial projects, optional credit. No AI per publisher. V1 uses prepared loop excerpts; original tracks preserved. No standalone music redistribution. |

[Fab Standard terms](https://www.fab.com/eula) permit commercial incorporated projects and private collaboration; standalone asset redistribution is prohibited. Personal and Professional tiers grant the same usage rights. Cached entitlement presence does not establish the exact historical acquisition agreement. Lead verified the repository is private through authenticated REST; all third-party audio and generated native audio assets nevertheless remain ignored pending per-pack publication clearance.

No new acquisition, payment, subscription, generation service, or AI audio model was used. Licensed audio was processed locally only, never supplied to an AI audio service. No CC-BY-SA, GPL or non-commercial sources are used.

The jam requires listing third-party assets even if attribution is optional. Lead owns root `CREDITS.md` / `DEVLOG.md`; this file and `sources.json` are the append handoff. Tools: Codex for implementation, existing local Python 3.13.9 / NumPy 2.3.4 for lossless payload recovery, ffmpeg / ffprobe 8.0.1 for preparation and QA, Unreal Engine 5.9.0 CL 60351404, Ryan's `ue-game-audio` skill at `4096f47`. No new synthesis is used.
