while getopts "p:c:" opt; do
  case $opt in
    p) PATH_PARAMETER="$OPTARG" ;;
    c) COMMAND_PARAMETER="$OPTARG" ;;
    *) usage ;;
  esac
done

if [ -z "$PATH_PARAMETER" ] || [ -z "$COMMAND_PARAMETER" ]; then
    usage
fi

listAll() {
    awk '
    BEGIN {
       FS="\\]";
    }
    {
        count = 0;
        for(i = 1; i <= NF; i++) {
            if ($i ~ /\[/) { count++; }
        }
        if(count == 7) {
            gsub(/^\[/, "", $1);
            gsub(/^\[/, "", $2);
            gsub(/^\[/, "", $3);
            gsub(/^\[/, "", $4);
            gsub(/^\[/, "", $5);
            gsub(/^\[/, "", $6);
            gsub(/^\[/, "", $7);
        
            printf "%s -> %s [Last Sync: %s] [%s]\n", $2, $3, $1, $6;
        }
    }' "$PATH_PARAMETER"
}

listMonitored() {
    grep "Monitoring started for" "$PATH_PARAMETER" | while read -r line; do
        timestamp=$(echo "$line" | grep -oP '\[\K[^\]]+')

        source_dir=$(echo "$line" | awk -F'for ' '{print $2}')

        target_dir=$(grep "Added directory: $source_dir ->" "$PATH_PARAMETER" | awk -F' -> ' '{print $2}' | tail -n 1)

        echo "$source_dir -> $target_dir [Last Sync: $timestamp]"
    done
}


listStopped() {
    grep "Monitoring stopped for" "$PATH_PARAMETER" | while read -r line; do
        timestamp=$(echo "$line" | grep -oP '\[\K[^\]]+')

        source_dir=$(echo "$line" | awk -F'for ' '{print $2}')

        target_dir=$(grep "Added directory: $source_dir ->" "$PATH_PARAMETER" | awk -F' -> ' '{print $2}' | tail -n 1)
        echo "$source_dir -> $target_dir [Last Sync: $timestamp]"
    done
}


purge() {
    if [ -d "$PATH_PARAMETER" ]; then
        echo "Deleting $PATH_PARAMETER..."
        rm -rf "$PATH_PARAMETER"
        echo "Purge complete."
    elif [ -f "$PATH_PARAMETER" ]; then
        echo "Deleting $PATH_PARAMETER..."
        rm -f "$PATH_PARAMETER"
        echo "Purge complete."
    else
        echo "$PATH_PARAMETER Not file or Directory."
    fi
}

case "$COMMAND_PARAMETER" in
    listAll)
        listAll;;
    listMonitored)
        listMonitored;;
    listStopped)
        listStopped;;
    purge)
        purge;;
    *)
      echo "Wrong command: $COMMAND_PARAMETER"
       usage;;
esac
