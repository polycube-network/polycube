#/usr/bin/env bash

# Implement polycubectl CLI autocompletion.
# Inspired by https://github.com/scop/bash-completion

parse_yaml() {
   local prefix=$2
   local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=$(echo @|tr @ '\034')
   sed -ne "s|^\($s\)\($w\)$s:$s\"\(.*\)\"$s\$|\1$fs\2$fs\3|p" \
        -e "s|^\($s\)\($w\)$s:$s\(.*\)$s\$|\1$fs\2$fs\3|p"  $1 |
   awk -F$fs '{
      indent = length($1)/2;
      vname[indent] = $2;
      for (i in vname) {if (i > indent) {delete vname[i]}}
      if (length($3) > 0) {
         vn=""; for (i=0; i<indent; i++) {vn=(vn)(vname[i])("_")}
         printf("%s%s%s=\"%s\"\n", "'$prefix'",vn, $2, $3);
      }
   }'
}

# Use bash-completion, if available
[[ $PS1 && -f /usr/share/bash-completion/bash_completion ]] && \
    . /usr/share/bash-completion/bash_completion

BASE_URL_="http://localhost:9000/polycube/v1/"

_polycubectl_completions() {
  if [ -a "$HOME/.config/polycube/polycubectl_config.yaml" ]
  then
    eval $(parse_yaml "$HOME/.config/polycube/polycubectl_config.yaml" "config_")
  fi

  BASE_URL=${POLYCUBECTL_URL:-$config_url}
  BASE_URL=${BASE_URL:-$BASE_URL_}
  local cur prev
  _get_comp_words_by_ref -n ":,=" cur prev
  local words
  _get_comp_words_by_ref -n ":,=" -w words

  OLDIFS=$IFS
  COMP=""

  URL0=""
  HELP_TYPE="NONE"
  i=0
  for X in ${words[@]}
  do
    let i++

    # do not consider the current word the user is typing
    #if [ $i == $((COMP_CWORD+1)) ]
    if [ "$X" == "$cur" ]
    then
      #break
      continue
    fi

    if [[ $X == *"="* ]]
    then
      continue
    fi

    if [[ $X == *":"* ]]
    then
      continue
    fi

    if [[ "polycubectl" == $X ]]
    then
      continue
    fi

    if [[ "add" == $X || "del" == $X || "show" == $X || "set" == $X ]]
    then
      HELP_TYPE=`echo $X | awk '{print toupper($0)}'`
      continue
    fi

    #echo $X
    URL0+=$X
    URL0+='/'
  done

  #if [ "${cur: -1}" == "=" ]
  if [[ $cur == *"="* ]]
  then
    URL0+="${cur%%=*}"
    URL0+='/'
  fi

  URL0="${URL0%?}"  # remove last '/'
  URL=$BASE_URL$URL0"?help="$HELP_TYPE"&completion"

  #echo "URL is: " $URL
  #return

  JSON=`curl -f -L -s -X OPTIONS $URL`
  if [ $? -ne 0 ]
  then
    return
  fi

  #echo $JSON

  VEC="`echo $JSON | jq -cr '.? // [] | @tsv'`"

  if [ ${#VEC[@]} == 1 ]
  then
    VEC0=${VEC[0]}
    if [[ ${VEC0:0:1} == "<" ]]
    then
      # if the command is ADD then print the keyname that is expected, but
      # without creating a completion
      printf "\n${VEC0}" >/dev/tty
      expandedPrompt=$(PS1="$PS1" debian_chroot="$debian_chroot" "$BASH" --norc -i </dev/null 2>&1 | sed -n '${s/^\(.*\)exit$/\1/p;}')
      printf '\n%s%s' "$expandedPrompt" "$COMP_LINE" >/dev/tty
      return 0;
    fi
  fi

  IFS=$'\t'
  for CMD in ${VEC[@]}
  do
    # add space for commands that doesn't end in "="
    if [ ${CMD: -1} != "=" ]
    then
      CMD+=" "
    fi

    if [[ $cur == *":"* ]]
    then
      CMD="${CMD#*:}"
    fi

    COMP+="$CMD"$IFS
  done

  IFS=$'\t\n'

  local cur0=$cur

  # if "=" or ":" are present in the current word, get the substring after it,
  # otherwise the completion is going to give wrong results
  if [[ $cur == *"="* ]]
  then
    cur0="${cur#*=}"
  fi

  if [[ $cur == *":"* ]]
  then
    cur0="${cur#*:}"
  fi

  COMPREPLY=( $(compgen -W "$COMP" -- $cur0) )

  IFS=$OLDIFS
}

complete -o nospace -F _polycubectl_completions polycubectl
