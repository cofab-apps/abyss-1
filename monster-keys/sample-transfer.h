/* SPDX-License-Identifier: MPL-2.0
 * USB MIDI SysEx, main loop. Samples have two seven-bit bytes each.
 * Build a take in the inactive bank and publish only a complete, checked transfer. */
static int transfer_id=-1, transfer_length, transfer_count, transfer_expected;
static uint16_t transfer_crc;
static volatile int import_ready;
static int unpack21(const uint8_t *p) { return p[0]|p[1]<<7|p[2]<<14; }

int app_sysex(const uint8_t *p,uint32_t n)
{
    if(n<6 || p[0]!=0x7d || p[1]!='S' || p[2]!='M' || p[3]!='P') return 0;
    int cmd=p[4], id=p[5], ok=0;
    if(cmd<1 || cmd>3) return 0;
    if(sample_mode && !sample_arm && !import_ready) {
        if(cmd==1 && n==12) {
            int len=unpack21(p+6);
            if(len>=CLIP_MIN && len<=CLIP_MAX) {
                transfer_id=id; transfer_length=len; transfer_count=0;
                transfer_expected=unpack21(p+9); transfer_crc=0xffff; ok=1;
            }
        } else if(cmd==2 && id==transfer_id && n>=11 && (n-9)%2==0 &&
                  unpack21(p+6)==transfer_count && (n-9)/2<=(unsigned)(transfer_length-transfer_count)) {
            for(unsigned i=9;i<n;i++) {
                transfer_crc^=(uint16_t)p[i]<<8;
                for(int b=0;b<8;b++)
                    transfer_crc=(uint16_t)((transfer_crc<<1)^((transfer_crc&0x8000)?0x1021:0));
            }
            for(unsigned i=9;i<n;i+=2)
                clips[sample_bank^1][transfer_count++]=((p[i]|p[i+1]<<7)-8192)*4;
            ok=1;
        } else if(cmd==3 && n==6 && id==transfer_id && transfer_count==transfer_length &&
                  transfer_crc==transfer_expected) {
            import_ready=transfer_count; transfer_id=-1; ok=1;
        }
    }
    uint8_t reply[]={0xf0,0x7d,'S','M','P',4,(uint8_t)id,(uint8_t)cmd,(uint8_t)ok,
        transfer_count&127,(transfer_count>>7)&127,(transfer_count>>14)&127,0xf7};
    plat_sysex_send(reply,sizeof(reply));
    redraw=1;
    return 1;
}
