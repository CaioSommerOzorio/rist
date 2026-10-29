# Rist
## Introduction
Rist is a command line tool built in C++ made to help linux users rice their setups.

## Installation
For now, clone the repository:
```bash
git clone https://github.com/CaioSommerOzorio/rist
cd rist
```
Then, you can build the executable from source if you'd like, and copy it to some PATH.
```bash
make # if you want
sudo cp build/rist /usr/bin/rist
```

## Usage
### Help
The help menu will come up by running `rist` by itself:
```
rist create riceName - creates a new rice environment and selects it
rist load riceName - loads target rice
rist destroy riceName - removes target rice
rist add directory - adds a directory to selected rice
rist rm directory - removes a directory from selected rice
```
### Creating and Adding to Rices
You can create a rice with the following command:
```
rist create riceName
```
Then, you might be interested in adding a config file to that rice. For example:
```
rist add ~/.config/i3/config
```
> Make sure you provide the absolute path

This will copy your `~/.config/i3/config` file somewhere else and create a symbolic link pointing to it in its place.

### Destroying and Removing Rices
You can destroy a rice with the following command:
```
rist destroy riceName
```
However, if you just want to get rid of a config file, use
```
rist rm ~/.config/i3/config
```

### Loading rices
Whatever command you run (e.g. adding/removing configs) will affect the currently loaded rice. Load a rice with
```
rist load riceName
```
