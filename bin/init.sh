#!/usr/bin/env bash
# Export necessary variables
export VFS_FILESYSTEM_PATH="$PWD"
export VFS_FILESYSTEM='versioning_filesystem'
# Add bin to path
export OLD_PATH="$PATH"
export PATH="$PATH$PWD/bin:"
# Backup aliases
save_alias() {
    local name="$1"
    if alias "$name" >/dev/null 2>&1; then
        # Capture the full alias definition string, e.g. "alias ls='ls -G'"
        local def
        def=$(alias "$name")
        export "OLD_ALIAS_$name=$def"
    fi
}
# save_alias() { # ZSH
#     local name="$1"
#     local var="_OLD_ALIAS_${name:u}"  # uppercase in zsh
#     if alias "$name" &>/dev/null; then
#         export "$var=$(alias "$name")"
#     else
#         export "$var=__NONE__"
#     fi
# }
# save_alias() { # BASH
#     local a="$1"
#     if alias "$a" &>/dev/null; then
#         export "_OLD_ALIAS_${a^^}"="$(alias "$a")"
#     else
#         export "_OLD_ALIAS_${a^^}"="__NONE__"
#     fi
# }
save_alias ls
save_alias touch
save_alias edit
save_alias cat
save_alias rm
save_alias res
# Add aliases
alias ls='vfs list'
alias touch='vfs create'
alias edit='vfs edit'
alias cat='vfs view'
alias rm='vfs remove'
alias res='vfs restore'