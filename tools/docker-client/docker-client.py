import docker
client = docker.from_env()

cnt=client.containers.list(all=True)
for i in cnt:
    print(i.attrs.get("Id"))