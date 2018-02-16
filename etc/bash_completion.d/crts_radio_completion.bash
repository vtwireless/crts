# In short just run:
#
#       source crts_radio_completion.bash
#
# and you get bash command line tab tab completion for the crts_radio
# program.


# This file can be sourced with your .bashrc file to get bash command line
# completion for the crts_radio program.  The bash completion will at
# least give to filter module plugin files for the plugins that you have
# installed.  If you add filter module plugins to the filter module plugin
# directory as "filename.so" this bash completion script will automatically
# add the module plugin to the filter module plugin bash completion list
# of the crts_radio program.

# This could also be installed proper. See
# https://serverfault.com/questions/506612/standard-place-for-user-defined-bash-completion-d-scripts
# is a good place to start.
#
# This may just get installed automatically, but with a "non-system"
# installation, you may need to think through how to install this.



# TODO: To get bash completion for the filter modules that are installed
# we assume that the CRTS filter plugins are in a directory that keeps a
# fixed relative path to this file.  In the future we could parse the
# users PATH and COMP_WORDS[0] and then figure out the path to the CRTS
# filter plugins from parsing those two strings.  That may be more work
# than this.


# Now this bash completion supports -l | --load G_MOD


# We want this to look in the CRTS filter modules plugin directory
# on-the-fly so that the user does not have to do anything, but put the
# plugin (DSO library file) into the CRTS filter modules plugin directory,
# to install plugins; and they are seen by this script and we get bash
# filter module name completion automatically.


function _crts_radio_complete()
{
    # This bash file is sourced and that not the same as running it.
    # The bash shell sourcing this needs to not get broken and polluted
    # by this code, so we need to minimise the residual resource usage
    # of this script.  This function only runs if the user runs
    # crts_radio, so it's not too bad.

    #echo "got COMP_LINE=${COMP_LINE}  COMP_POINT=${COMP_POINT} COMP_CWORD=$COMP_CWORD"
    #set | less
    local cur_word prev_word
    cur_word="${COMP_WORDS[COMP_CWORD]}"
    prev_word="${COMP_WORDS[COMP_CWORD-1]}"

    # COMPREPLY is the array of possible completions

    if [[ ${cur_word} == --f* ]] ; then
        # Just fill in: --filter
        COMPREPLY=("--filter")
        return 0 # done
    fi
    if [[ ${cur_word} == -f ]] ; then
        COMPREPLY=("-f")
        return 0 # done
    fi
    if [[ ${cur_word} == --l* ]] ; then
        # Just fill in: --load
        COMPREPLY=("--load")
        return 0 # done
    fi
    if [[ ${cur_word} == -l ]] ; then
        COMPREPLY=("-l")
        return 0 # done
    fi


    if [[ ${prev_word} != -f ]] && [[ ${prev_word} != "--filter" ]] &&\
        [[ ${prev_word} != -l ]] && [[ ${prev_word} != "--load" ]] ; then
        # We currently only do the filter and load option and this is
        # not that so:
        return 1 # default completion
    fi

    local mod_dir;

    # TODO: For now we require that this bash file be in a special path
    # directory which is requires that filter plugins be in
    # ../../share/crts/plugins/Filters/ relative to the directory this
    # file is in.
    #
    # This will even work when the software is not installed yet and
    # is built in the source directory.  A major major plus for
    # developers.

    if [[ ${prev_word} == -f ]] || [[ ${prev_word} == "--filter" ]] ; then
        mod_dir="$(dirname ${BASH_SOURCE[0]})" || return 0
        mod_dir="$mod_dir"/../../share/crts/plugins/Filters/
    else
        # it must be a -l | --load
        mod_dir="$(dirname ${BASH_SOURCE[0]})" || return 0
        mod_dir="$mod_dir"/../../share/crts/plugins/General/
    fi


    local i
    local mod
    local mods=()

    for i in $mod_dir/*.so ; do
        if [ ! -f "$i" ] ; then
            continue
        fi
        mod="$(basename ${i%%.so})"
        mods+=($mod)
        #echo "mod=${mod}"
    done

    #echo "mod_dir=$mod_dir"

    # COMPREPLY is the array of possible completions
    COMPREPLY=()
    for i in "${mods[@]}" ; do
        if [[ "$i" = "${cur_word}"* ]] ; then
            COMPREPLY+=($i)
        fi
    done

    return 0 # done
}

# Register the function _crts_radio_complete to provide bash completion for
# the program crts_radio
complete -o default -F _crts_radio_complete crts_radio
