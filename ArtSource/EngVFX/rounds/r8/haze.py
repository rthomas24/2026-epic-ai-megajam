#!/usr/bin/env python3
"""Round 10: M_TD_RegionHaze on a 600 m sphere (NS_TD_Debug_HazeSphere, review only). usage: python haze.py <tag>
(PIE on $TD_PORT running). Outside: surface 500 m ahead, chase camera and a side camera; inside: sphere on the ship."""
import os, sys, subprocess
H = os.path.dirname(os.path.abspath(__file__)); T = sys.argv[1]
SC = [sys.executable, os.path.join(H, 'spawncap.py')]
subprocess.check_call(SC + [T + '_haze_out_chase', 'NS_TD_Debug_HazeSphere', '800', 'chase'])
subprocess.check_call(SC + [T + '_haze_out_side', 'NS_TD_Debug_HazeSphere', '800', 'side', '1', '0', '{}', '1', '1', '60'])
subprocess.check_call(SC + [T + '_haze_in_chase', 'NS_TD_Debug_HazeSphere', '0', 'chase'])
