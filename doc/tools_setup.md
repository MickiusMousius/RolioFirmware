# Local ZMK v0.3 Development Environment Setup

Since ZMK main (pre-release) is targeting a different Zephyr version to ZMK 0.3 (stable) it can be a bit messy to do development against both versions of Zephyr & ZMK.

The instructions here are intended to help you set up a build environment that is tailored to ZMK 0.3 that won't cause conflicts with the ZMK main setup instructions.

If you wish to stick with ZMK main (pre-release) the [instructions on the ZMK site](https://zmk.dev/docs/development/local-toolchain/setup/container) are really quite good & should be used insteads of these.

> ⚠️ **NOTE:** This approach is for [VS Code](https://github.com/microsoft/vscode), not [Code OSS](https://github.com/microsoft/vscode/wiki/Differences-between-the-repository-and-Visual-Studio-Code).

## 1. Install Prerequisites

Before starting, ensure you have the required software installed on your machine:

- **[Docker Desktop](https://www.docker.com/products/docker-desktop)** for your operating system.
- **[VS Code](https://code.visualstudio.com/)**.
- **[Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)** for VS Code.

## 2. Prepare Local Directories & Docker Mounts

First, set up a dedicated workspace to store your ZMK v0.3 development files.

Create the necessary local directories:

```bash
mkdir zmk_0.3_local
cd zmk_0.3_local
mkdir zmk-config
mkdir zmk-modules
```

Next, configure Docker with labeled volumes that point specifically to your local ZMK 0.3 module and config paths:

```bash
docker volume create --driver local -o o=bind -o type=none -o device="/Users/ryan/GIT/zmk_0.3_local/zmk-config" zmk-config-v0.3
docker volume create --driver local -o o=bind -o type=none -o device="/Users/ryan/GIT/zmk_0.3_local/zmk-modules" zmk-modules-v0.3
```

> ⚠️ **NOTE:** I have used the path `/Users/ryan/GIT` in my example, you will need to use whatever path you've used on your development machine.

## 3. Clone ZMK Repository

Clone the ZMK firmware repository and check out the `v0.3-branch` to ensure you are working on the correct version:

```bash
git clone -b v0.3-branch git@github.com:zmkfirmware/zmk.git
cd zmk
```

## 4. Update ZMK Container Configuration

Update your Dev Container settings to use your ZMK 0.3 specific paths. This prevents you from poisoning your ZMK 4.1 development environment.

Edit `zmk/.devcontainer/devcontainer.json` to look exactly like this:

```json
{
  "name": "ZMK Development v0.3",
  "dockerFile": "Dockerfile",
  "runArgs": ["--security-opt", "label=disable"],
  "containerEnv": {
    "WORKSPACE_DIR": "${containerWorkspaceFolder}",
    "PROMPT_COMMAND": "history -a"
  },
  "mounts": [
    "type=volume,source=zmk-root-user-v0.3,target=/root",
    "type=volume,source=zmk-config-v0.3,target=/workspaces/zmk-config",
    "type=volume,source=zmk-modules-v0.3,target=/workspaces/zmk-modules",
    "type=volume,source=zmk-zephyr-v0.3,target=${containerWorkspaceFolder}/zephyr",
    "type=volume,source=zmk-zephyr-modules-v0.3,target=${containerWorkspaceFolder}/modules",
    "type=volume,source=zmk-zephyr-tools-v0.3,target=${containerWorkspaceFolder}/tools"
  ],
  "customizations": {
    "vscode": {
      "extensions": ["ms-vscode.cpptools"],
      "settings": {
        "terminal.integrated.shell.linux": "/bin/bash"
      }
    }
  },
  "forwardPorts": [3000]
}
```

## 5. Initialize the Container

Open the `zmk` checkout directory in VS Code. Because the repository includes Dev Container configurations, an alert will pop up:

Click **Reopen in Container** to launch VS Code inside the running container.

If the alert fails to pop up (or you accidentally close it), you can perform the same action by opening the Command Palette and selecting **Dev Containers: Rebuild and Reopen in Container**:

- **Windows/Linux**: `Ctrl + Shift + P`
- **macOS**: `Cmd + Shift + P`

> **Note:** The first time you do this, it will pull down the Docker image and build the container, which takes a few minutes. Subsequent launches will be much faster.

## 6. Configure the Zephyr Workspace

> ⚠️ **Caution:** The following steps and any future build commands must be executed from the terminal _inside_ the VS Code container.

Initialize West and update your modules:

```bash
west init -l app/ # Initialization
west update       # Update modules

```

Because you are using a Docker-based approach, you must restart the container at this point for the changes to take effect. You can restart it via the VS Code Command Palette, or stop it manually using your local host terminal:

```bash
docker ps                    # List running containers
docker stop <container-id>   # Stop the specific container

```

## 7. Configure and Open the Multi-Root Workspace

By default, the Dev Container will only show the main `zmk` repository in your file explorer. To view and edit your `zmk-config` and `zmk-modules` folders alongside it, you need to create and open a custom workspace configuration.

First, create a new file at `zmk/.devcontainer/zmk.code-workspace` and paste the following:

```json
{
  "folders": [
    {
      "path": ".."
    },
    {
      "path": "../../zmk-config"
    },
    {
      "path": "../../zmk-modules"
    }
  ],
  "settings": {
    "files.associations": {
      "*.overlay": "dts",
      "*.keymap": "dts"
    },
    "workbench.colorCustomizations": {
      "titleBar.activeBackground": "#5e2b97",
      "titleBar.activeForeground": "#ffffff",
      "titleBar.inactiveBackground": "#451e73",
      "statusBar.background": "#5e2b97",
      "statusBar.foreground": "#ffffff"
    }
  }
}
```

Next, tell VS Code to load this workspace file:

1. In the VS Code Explorer panel on the left, expand the `.devcontainer` folder.
2. Click on the `zmk.code-workspace` file to open it in the editor.
3. A button will appear (usually in the bottom-right corner or at the top of the editor pane) prompting you to open it as a workspace. Click **Open Workspace**.

- _(Alternatively, you can use the top menu: go to **File** > **Open Workspace from File...** and select `zmk/.devcontainer/zmk.code-workspace`)_

4. VS Code will quickly reload the window.

Once reloaded, your Explorer panel will display a "Multi-Root Workspace" containing three distinct folders: your main `zmk` repository, `zmk-config`, and `zmk-modules`. You can now easily navigate and edit all of your environment files in one place!

Congratulations! You should now have a fully isolated and working local development environment.
