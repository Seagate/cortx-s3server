from s3confstore.cortx_s3_confstore import S3CortxConfStore

key = 'cortx>software>s3>ipaddress'
opfileconfstore = S3CortxConfStore('yaml:///opt/seagate/cortx/s3/conf/s3.config.tmpl.1-node', 'read_idx')

ip_address = opfileconfstore.get_config(key)
print(ip_address)

ip_address = "http://" + ip_address + ":28049"
print(ip_address)
opfileconfstorenew = S3CortxConfStore('yaml:///opt/seagate/cortx/s3/s3backgrounddelete/config.yaml', 'write_idx')

opfileconfstorenew.set_config('cortx_s3>endpoint', ip_address, True)
