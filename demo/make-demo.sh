#!/bin/bash
# Turns a folder of screenshots AND short screen recordings into captioned stills and one
# reel.
#
# Each entry becomes a normalised segment — same size, frame rate and codec — so pictures and
# clips can sit in the same film without a re-encode at the join. Sources can be any size or
# shape: everything is scaled to fit a 1920x1080 frame and padded, so a tight capture of one
# jack and a wide shot of a rack sit together without either being distorted.
#
# Captions are rendered as images and composited, because this ffmpeg has no text filter built
# in — which turns out to be an advantage: real typography, a solid plate, any position.
set -e
cd "$(dirname "$0")"

W=1920; H=1080; FPS=30
OUT=out
rm -rf "$OUT"; mkdir -p "$OUT"
: > "$OUT/reel.txt"

# Scale to fit, pad to frame, lay the caption along the bottom.
CHAIN="[0:v]scale=$W:$H:force_original_aspect_ratio=decrease,\
pad=$W:$H:(ow-iw)/2:(oh-ih)/2:color=0x101216,fps=$FPS,format=yuv420p[bg];\
[bg][1:v]overlay=0:$((H-170))"

n=0
while IFS='|' read -r file secs caption; do
	file=$(echo "$file" | xargs); secs=$(echo "$secs" | xargs)
	caption=$(echo "$caption" | sed 's/^ *//; s/ *$//')
	[ -z "$file" ] && continue
	case "$file" in \#*) continue;; esac
	[ -f "shots/$file" ] || { echo "missing: shots/$file"; continue; }

	n=$((n+1))
	pad=$(printf "%03d" $n)

	safe=$(printf '%s' "$caption" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g')
	cat > "$OUT/cap-$pad.svg" <<EOF
<svg xmlns="http://www.w3.org/2000/svg" width="$W" height="150">
  <rect x="60" y="30" width="$((W-120))" height="90" rx="14" fill="#0b0d10" fill-opacity="0.94"/>
  <text x="96" y="88" font-family="Helvetica Neue, Helvetica, Arial" font-size="46"
        fill="#ffffff">$safe</text>
</svg>
EOF
	rsvg-convert -o "$OUT/cap-$pad.png" "$OUT/cap-$pad.svg"

	case "${file##*.}" in
		mov|mp4|m4v|MOV|MP4|M4V)
			# A clip: trimmed to `secs` if one is given, otherwise played in full.
			LIMIT=""; [ -n "$secs" ] && LIMIT="-t $secs"
			ffmpeg -hide_banner -loglevel error -y -i "shots/$file" -i "$OUT/cap-$pad.png" \
				-filter_complex "$CHAIN" $LIMIT -an -c:v libx264 -crf 18 -r $FPS \
				"$OUT/seg-$pad.mp4"
			;;
		*)
			# A still: held for `secs`, and also written out on its own for the forum post.
			ffmpeg -hide_banner -loglevel error -y -loop 1 -i "shots/$file" -i "$OUT/cap-$pad.png" \
				-filter_complex "$CHAIN" -t "${secs:-5}" -an -c:v libx264 -crf 18 -r $FPS \
				"$OUT/seg-$pad.mp4"
			ffmpeg -hide_banner -loglevel error -y -i "shots/$file" -i "$OUT/cap-$pad.png" \
				-filter_complex "$CHAIN" -frames:v 1 "$OUT/still-$pad.png"
			;;
	esac
	echo "file '$PWD/$OUT/seg-$pad.mp4'" >> "$OUT/reel.txt"
done < captions.txt

if [ -s "$OUT/reel.txt" ]; then
	# Every segment was encoded the same way, so the joins need no re-encode.
	ffmpeg -hide_banner -loglevel error -y -f concat -safe 0 -i "$OUT/reel.txt" \
		-c copy "$OUT/demo.mp4"
	echo "wrote $OUT/demo.mp4 from $n items, plus captioned stills in $OUT/"
else
	echo "nothing to do: add files to demo/shots and lines to demo/captions.txt"
fi
