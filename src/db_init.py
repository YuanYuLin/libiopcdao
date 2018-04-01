#!/bin/python2.7

import struct
import pprint

def create_buffer(record_count, record_size):
    buff=[]
    for idx in range(0, record_count):
        buff.append(bytearray('\0' * record_size))

    return buff

def dao_to_buffer(dao, buff, record_format, data_size):
    g_index=0
    for key in dao.keys():
        pad = bytearray(str.encode('\0' *  data_size))

        idx=0
        for ch in bytearray(key):
            pad[idx]=ch
            idx+=1

        for ch in bytearray(dao[key]):
            pad[idx]=ch
            idx+=1

        data = struct.pack(record_format, 0, 1, len(key), len(dao[key]), 0, str(pad))

        idx=0
        for ch in bytearray(data):
            buff[g_index][idx] = ch
            idx+=1
        g_index+=1

    return buff

def out_to_binary(buff):
    fd=open('db_init.bin', 'wb')
    for rec in buff:
        fd.write(rec)
    fd.close()

def out_to_c(buff, record_count, record_format):
    ft=open('db_init.inc', 'w')
    ft.write('#define DAO_KV_MAX ' + str(record_count) + '\n')
    ft.write('static struct dao_kv_512_t dao_kv_list[DAO_KV_MAX]={')
    idx=0
    for rec in buff:
        csum, tp, kl, vl, rsv, data = struct.unpack(record_format, rec)
        ft.write('\n' + '{')
        ft.write(str(int(csum)))
        ft.write(',')
        ft.write(str(int(tp)))
        ft.write(',')
        ft.write(str(int(kl)))
        ft.write(',')
        ft.write(str(int(vl)))
        ft.write(',')
        ft.write(str(int(rsv)))
        ft.write(',{')
        for ch in bytearray(data):
            ft.write(str(int(ch)))
            ft.write(',')
        ft.write('},')
        ft.write('},')
    ft.write('};')
    ft.close()

if __name__ == '__main__':
    dao={
"config_version":
    '{"major":0, "minor":0, "aux":0}',

"netifc_count":
    '["netifc_1", "netifc_2",  "netifc_3", "netifc_4"]',
"netifc_1":
    '{"visable":1, "editable":0, "type":"eth", "name":"eth0", "vlan":0, "tag":0, "devices":[], "src":"none"}',
"netifc_2":
    '{"visable":1, "editable":0, "type":"eth", "name":"eth0", "vlan":1, "tag":100, "devices":[], "src":"none"}',
"netifc_3":
    '{"visable":1, "editable":1, "type":"bridge", "name":"br0", "vlan":0, "tag":0, "devices":["eth0"], "src":"dhcp"}',
"netifc_4":
    '{"visable":1, "editable":1, "type":"bridge", "name":"br1", "vlan":0, "tag":0, "devices":["eth0.100"], "src":"storage", "net_cfg_json":"/hdd/sys/extra_cfg.json", "ctrlport":1}',

"storage_count":
    '["storage_1"]',
"storage_1":
    '{"device":"/dev/sda", "start":0, "end":262143, "sector_size":512, "media_type":"hdd", "partitions":["sda_part_1", "sda_part_2", "sda_part_3", "sda_part_4"]}',
"sda_part_1":
    '{"visable":1, "formatable":0, "type":"fat", "src":"/dev/sda1", "dst":"/hdd/sys", "label":"system", "start":2048, "sectors":102400}',
"sda_part_2":
    '{"visable":0, "formatable":0, "type":"sysdao", "src":"", "dst":"", "label":"sysdao1", "start":104448, "sectors":10240}',
"sda_part_3":
    '{"visable":0, "formatable":0, "type":"sysdao", "src":"", "dst":"", "label":"sysdao2", "start":114688, "sectors":10240}',
"sda_part_4":
    '{"visable":1, "formatable":0, "type":"fat", "src":"/dev/sda2", "dst":"/hdd/data", "label":"data", "start":124928, "sectors":262143}',

"lxc_count":
    '["lxc_1", "lxc_2", "lxc_3", "lxc_4"]',
"lxc_1":
    '{"name":"vm001", "rootfs":"/vms/vm001/rootfs", "fstab":"/vms/vm001/rootfs/fstab", "nettype":"veth", "nethwlink":"br1", "nethwaddr":"", "ipaddress":"", "gateway":""}',
"lxc_2":
    '{"name":"vm002", "rootfs":"/vms/vm002/rootfs", "fstab":"/vms/vm002/rootfs/fstab", "nettype":"veth", "nethwlink":"br1", "nethwaddr":"", "ipaddress":"", "gateway":""}',
"lxc_3":
    '{"name":"vm003", "rootfs":"/vms/vm003/rootfs", "fstab":"/vms/vm003/rootfs/fstab", "nettype":"veth", "nethwlink":"br1", "nethwaddr":"", "ipaddress":"", "gateway":""}',
"lxc_4":
    '{"name":"vm004", "rootfs":"/vms/vm004/rootfs", "fstab":"/vms/vm004/rootfs/fstab", "nettype":"veth", "nethwlink":"br1", "nethwaddr":"", "ipaddress":"", "gateway":""}',

"hostname_cfg":
    '{"src":"storage", "hostname_cfg_json":"/hdd/sys/extra_cfg.json"}',
"drbd_cfg":
    '{"ipaddress_local":"", "macaddress_local":"", "hostname_local":"qemu1", "drbd_local":"/dev/drbd0", "disk_local":"/dev/sdb", "hostname_remote":"qemu2", "drbd_remote":"/dev/drbd0", "disk_remote":"/dev/sdb", "ipaddress_remote":"", "macaddress_remote":""}',
    }
    header_size = 12
    data_size = 500
    record_count=128
    record_format="IBBHI" + str(data_size) + "s"
    record_size= header_size + data_size

    buf=create_buffer(record_count, record_size)
    buf=dao_to_buffer(dao, buf, record_format, data_size)
    out_to_binary(buf)
    out_to_c(buf, record_count, record_format)
