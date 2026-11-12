"""Attach collected engine evidence only after successful bounded UE processes."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
DIGEST=Path(r'C:\Program Files\Epic Games\UE_5.9\Engine\Extras\AgentTools\ue_log_digest.py')


def main():
    collected={}
    for stage in ('Build','Capture'):
        process=json.loads((HERE/f'Saved/{stage}.process.json').read_text(encoding='utf-8-sig'))
        result=json.loads((HERE/f'Saved/{stage}.result.json').read_text(encoding='utf-8-sig'))
        log=HERE/f'Saved/{stage}.log'
        completed=subprocess.run([sys.executable,'-B',str(DIGEST),str(log),'--json'],capture_output=True,text=True)
        digest=json.loads(completed.stdout)
        log_text=log.read_text(encoding='utf-8-sig')
        assert 'shaderplatform="PCD3D_SM6"' in log_text,stage+' did not use expected SM6'
        marker='HERO_M02_BUILD_SUCCESS' if stage=='Build' else 'HERO_M02_VERIFY_SUCCESS'
        assert marker in log_text,'Missing script completion marker: '+stage
        collected[stage]={'process':process,'engine_result':result,'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest(),
                          'digest':digest}
        assert process['exit_code']==0,process
        assert result['exit_code']==0 and result['reason']=='ok',result
        assert completed.returncode==0 and not digest['errors'],digest
        assert not result.get('suppressed_dialogs'),result.get('suppressed_dialogs')
    (HERE/'review/engine_diagnostics.json').write_text(json.dumps(collected,indent=2)+'\n')
    verification=HERE/'review/verification.json'
    report=json.loads(verification.read_text())
    report['external_engine_diagnostics']='engine_diagnostics.json: process exits, engine result files and complete log digest'
    report['shader_errors']=[error for stage in collected.values() for error in stage['digest']['errors']
                             if 'shader' in error.get('category','').lower()]
    report['engine_errors']=[error for stage in collected.values() for error in stage['digest']['errors']]
    report['engine_warnings']=[warning for stage in collected.values() for warning in stage['digest']['warnings']]
    report['diagnostics_verified']=True
    report['renderer']='DirectX 12 / Shader Model 6 (verified in both engine logs)'
    verification.write_text(json.dumps(report,indent=2)+'\n')
    print('Native diagnostic verification PASS; collected errors',len(report['engine_errors']),
          'warnings',len(report['engine_warnings']))


if __name__=='__main__':main()
