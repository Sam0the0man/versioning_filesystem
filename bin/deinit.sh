#!/usr/bin/env bash

# Remove exports
unset VFS_FILESYSTEM_PATH
unset VFS_FILESYSTEM

# Restore PATH
if [[ -n "$OLD_PATH" ]]; then
    export PATH="$OLD_PATH"
fi
unset OLD_PATH

# Restore aliases
restore_alias() {
    local name="$1"
    local var="OLD_ALIAS_$name"

    # Use bash indirect expansion or zsh ${(P)...}
    if [[ "$SHELL" == *bash* ]]; then
        old_def="${!var}"
    elif [[ "$SHELL" == *zsh* ]]; then
        old_def="${(P)var}"
    else
        echo "Unknown shell: $SHELL"
        return
    fi

    if [[ -n "$old_def" ]]; then
        # Stored value already contains "alias name='value'"
        eval "$old_def"
    else
        # No prior alias → remove the alias
        unalias "$name" 2>/dev/null
    fi

    unset "$var"
}

restore_alias ls
restore_alias touch
restore_alias edit
restore_alias cat
restore_alias rm
restore_alias res

