import psutil

def get_nic_statistics(interface):
    return psutil.net_io_counters(pernic=True)[interface]

#>>> psutil.net_io_counters(pernic=True)
#{'lo': snetio(bytes_sent=17373480, bytes_recv=17373480, packets_sent=156248, packets_recv=156248, errin=0, errout=0, dropin=0, dropout=0),
# 'enp1s0f0': snetio(bytes_sent=3844614, bytes_recv=17377, packets_sent=12109, packets_recv=75, errin=0, errout=0, dropin=0, dropout=0),
# 'enp3s0': snetio(bytes_sent=7709139, bytes_recv=1029300, packets_sent=47949, packets_recv=17155, errin=0, errout=0, dropin=0, dropout=0),
# 'enp0s31f6': snetio(bytes_sent=1456514469, bytes_recv=1215442938, packets_sent=1807625, packets_recv=3358297, errin=0, errout=0, dropin=254, dropout=0)}
