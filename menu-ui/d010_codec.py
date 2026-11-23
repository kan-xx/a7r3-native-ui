"""Pure D010 resident menu codec. No transport/camera/filesystem mutation."""
import struct
HEADER=1280
REPORT=1148
CHUNK=8192
SNAPSHOT=29968
BUILD=0x3110d015
OPS={1:'install',2:'query',3:'remove_request',4:'drain_backend_guard_keep_provider_pin',5:'diagnostic_once',6:'diagnostic_continue',7:'diagnostic_snapshot'}
PHASE={0:'cold',1:'installing',2:'resident',3:'removing',4:'stopped_pinned',5:'hold_pinned'}
DIAG={0:'idle',1:'baseline',2:'AA_open',3:'own_open',4:'script',5:'pause_own_for_pixels',6:'own_close',7:'reopen',8:'stock_select',9:'stock_open',10:'pause_stock_for_pixels',11:'stock_return',12:'restore_selection',13:'AA_close',14:'done',15:'cleanup',16:'hold'}
def bind(owner_tid,instance,nonce,backend_kind=0):
 if not 0<owner_tid<2**32 or backend_kind not in (0,1,2) or not isinstance(instance,bytes) or len(instance)!=16 or not any(instance) or not isinstance(nonce,bytes) or len(nonce)!=16 or not any(nonce):raise ValueError('fixed owner/backend/instance/nonce')
 return struct.pack('<4I',1,48,owner_tid,backend_kind)+instance+nonce

def request(op,nonce,offset=0,index=None,binding=None):
 if op not in OPS or not isinstance(nonce,bytes) or len(nonce)!=16 or not any(nonce):raise ValueError('operation and full nonce')
 if op==7:
  if index not in range(4) or not 0<=offset<SNAPSHOT or binding is not None:raise ValueError('snapshot bounded index/offset')
 elif offset or index is not None:raise ValueError('offset/index only snapshot')
 if op==1:
  if not isinstance(binding,bytes) or len(binding)!=48 or struct.unpack_from('<2I',binding)!=(1,48) or not struct.unpack_from('<I',binding,8)[0] or struct.unpack_from('<I',binding,12)[0] not in (0,1,2) or not any(binding[16:32]) or binding[32:]!=nonce:raise ValueError('strict bind48')
 elif binding is not None:raise ValueError('binding only install')
 return struct.pack('<HH',0,0xd010)+b'A7PRD7S1'+struct.pack('<II',op,offset)+nonce+(binding or b'')+(struct.pack('<I',index) if op==7 else b'')

def reply(data,op,nonce,offset=0,index=None):
 if len(data)<HEADER or len(data)>HEADER+CHUNK or data[:8]!=b'A7PRD7R1' or data[48:64]!=nonce:raise ValueError('magic/nonce/length')
 h=dict(zip(['version','op','status','report_bytes','snapshot_index','offset','chunk_bytes','total','flags','build'],struct.unpack_from('<10I',data,8)))
 if h['version']!=1 or h['op']!=op or h['report_bytes']!=REPORT or h['offset']!=offset or h['build']!=BUILD or h['flags']&~3:raise ValueError('wire identity')
 if h['chunk_bytes']>CHUNK or len(data)!=HEADER+h['chunk_bytes'] or any(data[64+REPORT:HEADER]):raise ValueError('chunk/reserved tail')
 if op==7:
  if index not in range(4) or h['snapshot_index']!=index or h['total']!=SNAPSHOT or offset+h['chunk_bytes']>SNAPSHOT:raise ValueError('snapshot extent')
 elif h['snapshot_index']!=0xffffffff or h['total'] or h['chunk_bytes']:raise ValueError('unexpected data')
 b=memoryview(data)[64:64+REPORT]
 def words(names,at):
  keys=names.split();return dict(zip(keys,struct.unpack_from('<'+str(len(keys))+'I',b,at)))
 r=words('version bytes phase error pid owner reference callbacks queued registry_added registry_removed registry_node factory_calls factory_rejected factory_lr object vptr view_live records_allocated records_freed backend_guard callback_identity stop_requested backend_kind',0)
 r['native']=words('bind_stage obj_base manager expected_return map_count map_lines map_bytes map_error',96)
 r['route']=words('version status prepared installed restored reads bytes checks aa root leaf last_address last_n conflict',128)
 p=words('version owner serial opens exits active close_queued original_queued event_count ticks draws requests error',184);p['options']=words('active row held pending last_event ignored',236);r['page']=p
 a=words('version state owner_tid request_seq asset target accepted completed code control old_generation ticks calls query_fresh observed_desired qualified gate automatic new_generation',260);a['nonce']=bytes(b[336:352]).hex();a['capture']=words('version bytes phase error bound base entry source_generation source_pid floor last_order arm_control offer_control armed_serial observed eligible accepted skipped old duplicates busy audio_phase audio_result dropped bad_message last_capture last_sequence last_media last_tid last_revoke held_reference source_matched',352);a['observed_control']=struct.unpack_from('<I',b,480)[0];r['audio']=a
 r['graphics']=words('version stage status exception uncertain owner_tid create_tid destroy_tid thread_errors pre_valid post_valid restored layer screen label font_id constructed active destructed dirty before_layers before_screens after_layers after_screens reads bytes',484)
 r['view']=words('ctor entry exit main action onload onunload dtor deleting last_event layout_open layout_close layer owner_tid closed entry_tid dtor_tid thread_errors',588)
 r['stock']=words('attempted returned exception',660)
 d=words('version stage error ticks opens aa aa_vptr aa_owned events constructed destroyed event_exceptions key_index requests_before sequence_base continued stock_base stock_func stock_base_id stock_func_id snapshots restored final_aa_count baseline_flag',672)
 d['route']=words('version status step reads bytes nodes matches root tab page leaf old_tab old_page old_leaf selected restored route_installed route_restored action_called action_returned exception flag_before flag_after flag_restored last_address last_n flag_restore_write',768)
 d['receipts']=[]
 for i in range(4):
  x=words('sequence asset target accepted completed code control old_generation new_generation qualified gate automatic',876+i*64);x['nonce']=bytes(b[924+i*64:940+i*64]).hex();d['receipts'].append(x)
 r['diagnostic']=d;r['nonce']=bytes(b[1132:1148]).hex()
 if r['version'] and (r['version']!=1 or r['bytes']!=REPORT or r['nonce']!=nonce.hex()):raise ValueError('core report identity')
 r['phase_name']=PHASE.get(r['phase'],'unknown');d['stage_name']=DIAG.get(d['stage'],'unknown')
 r['resident_installed']=h['status']==0 and r['phase']==2 and r['reference']==1 and r['backend_guard']==1 and r['registry_added']==1 and r['registry_removed']==0 and r['route']['installed']==1 and r['route']['restored']==0 and r['callback_identity']==1 and r['backend_kind'] in (0,1,2)
 receipts=d['receipts']
 def valid_receipt(i,x):
  common=x['sequence']==d['sequence_base']+i+1 and x['asset']==[0,2,3,0][i] and x['target']==[0,1,2,0][i] and x['accepted']==x['completed']==1 and not x['code'] and any(bytes.fromhex(x['nonce']))
  # OFF and the empty slot are valid when closed/drained even with no new card generation. It
  # may alternatively complete a full fresh qualified desired0 transaction.
  closed_off=i in(0,3) and (x['gate']&3)==1
  fresh=x['qualified']==1 and not(x['gate']&3) and x['new_generation']>x['old_generation']
  return common and (closed_off or fresh)
 rv=all(valid_receipt(i,x) for i,x in enumerate(receipts))
 rv=rv and len({x['nonce'] for x in receipts})==4 and all(receipts[i]['control']<receipts[i+1]['control'] for i in range(3))
 d['audio_receipts_valid']=rv
 d['success']=r['resident_installed'] and d['stage']==14 and not d['error'] and d['opens']==2 and d['restored']==1 and d['aa_owned']==1 and not d['final_aa_count'] and d['events']==d['constructed']==d['destroyed']==30 and not d['event_exceptions'] and d['continued']==2 and rv and p['requests']==d['requests_before']+4 and d['route']['selected']==3 and d['route']['restored']==1 and not d['route']['route_installed'] and d['route']['action_called']==d['route']['action_returned']==1 and d['route']['version']==5 and d['route']['flag_after'] in (0,1) and d['route']['flag_restore_write']==0 and not d['route']['exception']
 r['removed_and_backend_guard_released']=h['status']==0 and r['phase']==4 and r['reference']==1 and r['backend_guard']==0 and r['registry_removed']==1 and r['route']['restored']==1 and not r['view_live'] and not r['queued'] and r['records_allocated']==r['records_freed'] and not r['graphics']['uncertain']
 r['provider_unloaded']=False;r['physical_button_tested']=False;r['cold_boot_tested']=False;r['LCD_pixels_proved_by_this_reply']=False;r['backend_automatic_admission']=bool(a['automatic']);r['continuous_shutter_link_accepted']=False;r['menu_arms_capture']=False
 h['report']=r;return h
