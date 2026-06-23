import struct
EXPECTED=['00500113','00C00193','FF718393','0023E233','0041F2B3','004282B3',
'02728863','0041A233','00020463','00000293','0023A233','005203B3','402383B3',
'0471AA23','06002103','005104B3','008001EF','00100113','00910133','0221A023','00210063']
d=open('riscvtest.bin','rb').read()
ok=True
for i in range(0,len(d),4):
    w=struct.unpack('<I',d[i:i+4])[0]
    e=int(EXPECTED[i//4],16)
    if w!=e: ok=False; print(f'{i:#06x}: {w:08X} != {EXPECTED[i//4]}')
print('riscvtest: machine code coincide con el oficial' if ok else 'DIFERENCIAS')
