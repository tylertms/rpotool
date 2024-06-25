import ei_pb
import base64
import requests
import zlib

def get_config() -> ei_pb.ConfigResponse:
    config_request = ei_pb.ConfigRequest()
    config_request.rinfo.ei_user_id = input("EID: ")
    config_request.rinfo.client_version = 127
    
    data = {'data': base64.b64encode(config_request.SerializeToString())}
    request = requests.post("https://auxbrain.com/ei/get_config", data=data)

    authenticated_response = ei_pb.AuthenticatedMessage()
    authenticated_response.ParseFromString(base64.b64decode(request.content))

    inflated_message = zlib.decompress(authenticated_response.message)

    config_response = ei_pb.ConfigResponse()
    config_response.ParseFromString(inflated_message)

    return config_response

def parse_config():
    config = get_config()
    dlc_catalog = config.dlc_catalog

    with open("shells.txt", "w") as f:
        sets = {}
        asset_types = []
        chicken_types = []

        for shell_set in dlc_catalog.shell_sets:
            shell_set.name = shell_set.name
            sets[shell_set.identifier] = shell_set.name
            # f.write(f"set,{shell_set.identifier},{shell_set.name}\n")

        for decorator in dlc_catalog.decorators:
            decorator.name = decorator.name
            sets[decorator.identifier] = decorator.name
            # f.write(f"decorator,{decorator.identifier},{decorator.name}\n")
    
        for shell in dlc_catalog.shells:
            set_name = "NO SET"
            if shell.set_identifier in sets:
                set_name = sets[shell.set_identifier]
            elif shell.name is not None:
                set_name = shell.name

            type = ei_pb.ShellSpec.AssetType.Name(shell.primary_piece.asset_type)
            url_key = shell.primary_piece.dlc.url.split("_")[-1].split(".")[0]
            original_size = shell.primary_piece.dlc.original_size

            if type not in asset_types:
                asset_types.append(type)
            
            if len(url_key) != 32:
                print(f"Invalid URL Key: {url_key}")
                continue

            f.write(f"shell|{set_name} - {type}|{shell.identifier}|{url_key}|{original_size}\n")

        for shell_object in dlc_catalog.shell_objects:

            type = ei_pb.ShellSpec.AssetType.Name(shell_object.asset_type)
            object_name = shell_object.name

            if object_name not in chicken_types:
                chicken_types.append(object_name)

            for piece in shell_object.pieces:
                url_key = piece.dlc.url.split("_")[-1].split(".")[0]
                dlc_name = piece.dlc.name

                if len(url_key) != 32:
                    dlc_name = str.join("_", piece.dlc.url.split("_")[:-1]).split("/")[-1]

                f.write(f"chicken|{object_name}|{dlc_name}|{url_key}|{piece.dlc.original_size}\n")
        

if __name__ == "__main__":
    parse_config()