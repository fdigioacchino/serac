import importlib
import yaml

def get_specs_for(machine_name, using_yaml):
    specs = []
    if using_yaml:
        with open("build_config.yaml", "r") as stream:
            try:
                specs = yaml.safe_load(stream)[machine_name]
                for i, val in enumerate(specs):
                    specs[i] = "%" + "".join(val)
            except yaml.YAMLError as exc:
                print(exc)
    else:
        config_module = importlib.import_module("build_config")
        specs = config_module.specs()[machine_name]
        specs = ['%' + spec for spec in specs]

    return specs

if __name__ == "__main__":
    machine_name = "toss_4_x86_64_ib"

    for spec in get_specs_for(machine_name, True):
        print(spec)
