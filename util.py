from json.decoder import JSONDecodeError
from typing import List, Optional, Tuple
from dataclasses import dataclass
from shlex import split
from subprocess import run, PIPE, STDOUT
import re
import warnings
import json

@dataclass
class ArduinoPortInfo:
    """ Represents info about an Arduino device connected through a serial port"""
    port: str
    FQBN: str
    protocol: str = None
    type: str = None
    boardName: str = None
    core: str = None

def getArduinoPorts() -> List[ArduinoPortInfo]:
    """
        Returns a list of Arduino devices connected to the computer.
        Requires [arduino-cli](https://github.com/arduino/arduino-cli) to be installed and added to the PATH
    """

    lines = bash('arduino-cli board list').split('\n')
    headers = lines[0]
    lines = [line for line in lines[1:] if len(line) == len(headers)]

    headerMatches = re.finditer("(\w+) +", headers)
    headerMatches = [(x.span(), x.group()) for x in headerMatches]

    # Beacause "Board Name" header has space inside, we need to merge it's match with the next one
    try:
        it = enumerate(headerMatches)
        while True:
            i, ((start, end), string) = next(it)
            if "Board" in string:
                j, ((_, nextEnd), nextString) = next(it)
                headerMatches[i] = ((start, nextEnd), string + nextString)
                del headerMatches[j]
                break

    except StopIteration:
        pass

    ports = []

    for line in lines:
        values = []
        for match in headerMatches:
            start, end = match[0]
            values.append(line[start:end].strip())

        ports.append(ArduinoPortInfo(
            port = values[0],
            protocol = values[1],
            type = values[2],
            boardName = values[3],
            FQBN = values[4],
            core = values[5]
        ))

    return ports

def getAndUpdateArduinoPort(configFilePath: str) -> Tuple[bool, ArduinoPortInfo]:
    """
        Gets a port info about a connected Arduino board, or reads the info from `./vscode/arduino.json`
        when no board is present. When a connected board is selected and it its not the one saved in `arduino.json`,
        it updates the json file
    """

    config = dict()
    ports = getArduinoPorts()

    try:
        with open(configFilePath) as json_file:
            config = json.load(json_file)

        if not ports:
            return False, ArduinoPortInfo(
                port=config['port'],
                FQBN=config['board']
            )

        matchingPorts = [p for p in ports if p.port == config['port'] and p.FQBN == config['board']]

        if matchingPorts:
            return True, matchingPorts[0]

        warnings.warn("Port and FQBN found in config does not match any of the connected boards. Updating config...")

    except IOError:
        warnings.warn("Arduino config file could not be opened")
    except JSONDecodeError as e:
        warnings.warn("Arduino config file is invalid json", e)

    port = ports[0]

    if len(ports) > 1:
        warnings.warn(f"Multiple arduino boards identified. Using the first one ({port})")

    if config:
        config['port'] = port.port
        config['board'] = port.FQBN

        try:
            with open(configFilePath, 'w') as json_file:
                json.dump(config, json_file)

        except IOError:
            warnings.warn("Arduino config file could not be opened")

    return True, port

def bash(cmd, *args) -> str:
    """ Executes the specified command and returns it's output as a string"""

    argv = [cmd]
    argv.extend(args)

    if len(args) == 0 and type(cmd) is str:
        argv = split(cmd)

    return run(argv, stdout=PIPE, stderr=STDOUT, check=True).stdout.decode('UTF-8')
