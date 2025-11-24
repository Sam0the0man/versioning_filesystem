#!/usr/bin/env bash
# Remove exports
unset VFS_FILESYSTEM_PATH
unset VFS_FILESYSTEM
# Remove bin from path
export PATH="$OLD_PATH"
unset OLD_PATH
# Restore aliases
restore_alias() {
    local name="$1"
    local var="OLD_ALIAS_$name"

    if [[ "$SHELL" == *bash* ]]; then
        local old_def="${!var}" # BASH
    elif [[ "$SHELL" == *zsh* ]]; then
        local old_def="${(P)var}" # ZSH
    else
        echo "Shell type - "$SHELL" - not matched..."
        return
    fi

    if [ -n "$old_def" ]; then
        # Evaluate the stored alias definition
        # eval "alias $var"
        eval "alias $old_def"
        unset "$var"
    else
        # If no backup exists, remove the alias entirely
        unalias "$name" 2>/dev/null
    fi
}
restore_alias ls
restore_alias touch
restore_alias edit
restore_alias cat
restore_alias rm
restore_alias res