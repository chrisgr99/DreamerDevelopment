#!/usr/bin/env python3
"""Renders one video per feature, from the same scenes the manual animates.

  python3 docs/images/make-video.py

Drawn at 1080p rather than scaled up from the animations: everything is SVG, so the video is
redrawn at video size and nothing is enlarged. Each section is its own file — a question in a
forum thread is better answered with the twenty seconds that covers it than with a long video
and a timestamp — and the sections are made to be strung together afterwards.
"""
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import rackart as art
import clips


HERE = Path(__file__).parent
OUT = HERE.parent.parent / "demo" / "clips"
NARRATION = HERE.parent.parent / "demo" / "narration.txt"
VOICE = "Karen (Premium)"
# Words a minute. The default is 175; this is a shade under ten per cent faster, which comes
# off the running time without sounding hurried. Done in the voice rather than by stretching
# the audio afterwards, so the rhythm of the sentences survives.
VOICE_RATE = "190"
# How long the picture stays after the voice has finished. Long enough to breathe, and no
# longer: at two seconds, twelve sections spent twenty-four seconds of the film on nothing
# happening.
TAIL_SECONDS = 0.6
VIDEO_W, VIDEO_H = 1920, 1080
FPS = 12                 # What the scenes were timed for; the file is written at 30.
CARD_SECONDS = 1.6
# A beat after the feature has been named, before the demonstration starts. The name and the
# first sentence of explanation are two different thoughts, and running them together gives
# the viewer no moment to take the name in.
CARD_PAUSE = 1.0


def card(title, line):
    """A feature's title card, drawn on the panel's own background."""
    body = "\n  ".join([
        f'<text x="{VIDEO_W / 2}" y="{VIDEO_H / 2 - 30}" text-anchor="middle" '
        f'font-family="Nunito, Helvetica, Arial, sans-serif" font-weight="700" '
        f'font-size="86" fill="#e6e8ec">{title}</text>',
        f'<text x="{VIDEO_W / 2}" y="{VIDEO_H / 2 + 40}" text-anchor="middle" '
        f'font-family="Helvetica, Arial, sans-serif" font-size="38" fill="#9aa1ac">{line}</text>',
        f'<rect x="{VIDEO_W / 2 - 90}" y="{VIDEO_H / 2 + 84}" width="180" height="3" '
        f'fill="#3de07a"/>',
    ])
    return art.svg(VIDEO_W, VIDEO_H, body)


def narration_for(slug):
    """The phrases for a section, in order, from demo/narration.txt.

    One phrase per line, keyed by the name without its leading number so sections can be
    renumbered without renaming every block."""
    slug = slug.split("-", 1)[1] if slug[0].isdigit() else slug
    if not NARRATION.exists():
        return []
    lines, want = [], False
    for line in NARRATION.read_text().splitlines():
        if line.startswith("[") and line.endswith("]"):
            want = (line[1:-1] == slug)
            continue
        if want and line.strip() and not line.startswith("#"):
            lines.append(line.strip())
    return lines


def speak(text, name):
    """Renders one phrase and returns (audio path, seconds).

    THE VOICE SETS THE PACE, phrase by phrase. One recording for a whole section can only be
    laid over the pictures and hoped for; a recording per phrase lets each sentence begin
    where the thing it describes begins, which is the only way a demonstration reads as an
    explanation rather than a commentary.
    """
    OUT.mkdir(parents=True, exist_ok=True)
    aiff = OUT / f"_{name}.aiff"
    audio = OUT / f"_{name}.wav"
    # Half a second of silence in front of every phrase. Sentences that arrive back to back
    # are hard to follow even when each one is clear on its own.
    subprocess.run(["say", "-v", VOICE, "-r", VOICE_RATE, "-o", str(aiff),
                    "[[slnc 500]] " + text], check=True)
    # WAV until the very end. AAC in an MP4 carries priming samples, and joining such files
    # end to end left the finished track running at twice the length of the pictures.
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(aiff),
                    "-c:a", "pcm_s16le", "-ar", "44100", "-ac", "1", str(audio)], check=True)
    aiff.unlink()
    out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                          "-of", "csv=p=0", str(audio)], capture_output=True, text=True)
    return audio, float(out.stdout.strip())


def pad_to(audio, seconds, out):
    """The same speech with silence after it, so a phrase occupies exactly the stretch of
    pictures it belongs to and the next one starts on the next action."""
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(audio),
                    "-af", "apad", "-t", f"{seconds:.3f}",
                    "-c:a", "pcm_s16le", "-ar", "44100", "-ac", "1", str(out)], check=True)
    return out


def silence(seconds, out):
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-f", "lavfi",
                    "-i", "anullsrc=r=44100:cl=mono", "-t", f"{seconds:.3f}",
                    "-c:a", "pcm_s16le", str(out)], check=True)
    return out


def join_audio(parts, out):
    """Joined with the concat FILTER, which re-times the samples, rather than the demuxer,
    which trusts each file's own timestamps and gets them wrong for a run of clips."""
    cmd = ["ffmpeg", "-y", "-loglevel", "error"]
    for p in parts:
        cmd += ["-i", str(p)]
    streams = "".join(f"[{i}:a]" for i in range(len(parts)))
    cmd += ["-filter_complex", f"{streams}concat=n={len(parts)}:v=0:a=1[a]",
            "-map", "[a]", "-c:a", "aac", "-b:a", "160k", str(out)]
    subprocess.run(cmd, check=True)
    for p in parts:
        p.unlink()
    return out


def title_card():
    """The opening card: the name, what it is for, and the two faceplates.

    Test Gear stands beside Clarity because it is in every screenshot of this rack, and a
    module nobody has explained is a question the viewer carries for the rest of the video.
    """
    panels = art.embedded(HERE / "panels-both.jpg")
    panel_h = 820.0
    panel_w = panel_h * 364.0 / 715.0
    panel_x = VIDEO_W - panel_w - 150.0
    # Centred in the space it actually has: the words own everything to the left of the
    # faceplates, so they sit in the middle of THAT rather than in the middle of the frame.
    mid = panel_x / 2.0
    body = "\n  ".join([
        f'<image href="{panels}" x="{panel_x:.0f}" '
        f'y="{(VIDEO_H - panel_h) / 2:.0f}" width="{panel_w:.0f}" height="{panel_h:.0f}"/>',
        f'<text x="{mid:.0f}" y="{VIDEO_H / 2 - 70}" text-anchor="middle" '
        f'font-family="Nunito, Helvetica, Arial, sans-serif" '
        f'font-weight="700" font-size="104" fill="#e6e8ec">Clarity</text>',
        f'<text x="{mid:.0f}" y="{VIDEO_H / 2 - 12}" text-anchor="middle" '
        f'font-family="Helvetica, Arial, sans-serif" '
        f'font-size="36" fill="#9aa1ac">by Dreamer Development</text>',
        f'<rect x="{mid - 90:.0f}" y="{VIDEO_H / 2 + 26}" width="180" height="3" fill="#3de07a"/>',
        f'<text x="{mid:.0f}" y="{VIDEO_H / 2 + 96}" text-anchor="middle" '
        f'font-family="Helvetica, Arial, sans-serif" '
        f'font-size="34" fill="#e6e8ec">Designed to bring visual and interaction</text>',
        f'<text x="{mid:.0f}" y="{VIDEO_H / 2 + 142}" text-anchor="middle" '
        f'font-family="Helvetica, Arial, sans-serif" '
        f'font-size="34" fill="#e6e8ec">clarity to your rack</text>',
        f'<text x="{mid:.0f}" y="{VIDEO_H - 90}" text-anchor="middle" '
        f'font-family="Helvetica, Arial, sans-serif" font-size="26" fill="#3de07a">'
        f'github.com/chrisgr99/DreamerDevelopment</text>',
    ])
    return art.svg(VIDEO_W, VIDEO_H, body)


def encode(frames, out, audio):
    """Frames to an MP4, with the narration alongside if there is any."""
    cmd = ["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(FPS),
           "-i", str(frames / "%04d.png")]
    if audio:
        cmd += ["-i", str(audio), "-c:a", "aac", "-b:a", "160k"]
    # faststart puts the index at the front of the file, which is what lets a player show
    # something before it has the whole thing.
    cmd += ["-vf", f"pad={VIDEO_W}:{VIDEO_H}:(ow-iw)/2:(oh-ih)/2:color=0x161a20,fps=30",
            "-c:v", "libx264", "-preset", "slow", "-crf", "18", "-pix_fmt", "yuv420p",
            "-profile:v", "high", "-level", "4.0",
            "-g", "15", "-movflags", "+faststart", str(out)]
    subprocess.run(cmd, check=True)
    for f in frames.glob("*.png"):
        f.unlink()
    frames.rmdir()
    if audio:
        audio.unlink()
    print("wrote", out.name)


def hold(frames, source, count, start):
    """Repeats a frame, which is how a picture waits for the voice to catch up."""
    for n in range(start, start + count):
        subprocess.run(["cp", str(source), str(frames / f"{n:04d}.png")], check=True)
    return start + count


def render_title():
    """The opening. One picture throughout, but paced like every other section: a phrase at a
    time, each with the same half second in front of it."""
    phrases = narration_for("title")
    frames = OUT / "_title"
    frames.mkdir(parents=True, exist_ok=True)
    art.render(title_card(), frames / "0001.png", width=VIDEO_W)

    parts, held = [], 0.0
    for i, phrase in enumerate(phrases):
        spoken_audio, spoken = speak(phrase, f"title-{i}")
        parts.append(pad_to(spoken_audio, spoken, OUT / f"_title-{i}-fit.wav"))
        spoken_audio.unlink()
        held += spoken
    held = max(4.5, held + TAIL_SECONDS)
    parts.append(silence(max(0.1, held - sum_of(parts)), OUT / "_title-tail.wav"))

    hold(frames, frames / "0001.png", int(held * FPS) - 1, 2)
    audio = join_audio(parts, OUT / "_title-full.m4a") if parts else None
    encode(frames, OUT / "clarity-1-title.mp4", audio)


def sum_of(parts):
    total = 0.0
    for p in parts:
        out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                              "-of", "csv=p=0", str(p)], capture_output=True, text=True)
        total += float(out.stdout.strip())
    return total


def render_section(slug, title, line, builders, loops=1):
    """A section, assembled phrase by phrase.

    Each phrase owns a stretch of pictures: the first owns the title card, and each one after
    it owns a clip. The pictures of that stretch repeat until the phrase has been said, and
    the phrase is padded with silence to exactly that length — so the next sentence starts
    where the next action starts, however long either of them happens to be.
    """
    phrases = narration_for(slug)
    frames = OUT / f"_{slug}"
    pool = frames / "pool"
    pool.mkdir(parents=True, exist_ok=True)

    # Every distinct picture is drawn ONCE into a pool; the sequence is assembled by copying.
    art.render(card(title, line), pool / "card.png", width=VIDEO_W)
    stretches = [[pool / "card.png"] * int(CARD_SECONDS * FPS)]
    stills = [True]
    for b, build in enumerate(builders):
        clip = build()
        scale = min(VIDEO_W / clip.width, VIDEO_H / clip.height)
        width = int(clip.width * scale)
        shot = []
        for i, scene in enumerate(clip.scenes):
            path = pool / f"{b:02d}-{i:04d}.png"
            art.render(scene, path, width=width)
            shot.append(path)
            if clip.still:
                break          # One frame is the whole picture.
        stretches.append(shot if clip.still else shot * loops)
        stills.append(clip.still)

    # Phrases left over after the last clip belong to it as well: it repeats while they are
    # said, rather than freezing.
    while len(phrases) > len(stretches):
        stretches.append(list(stretches[-1]))
        stills.append(stills[-1])

    sequence, audio_parts = [], []
    for i, stretch in enumerate(stretches):
        phrase = phrases[i] if i < len(phrases) else None
        if phrase:
            spoken_audio, spoken = speak(phrase, f"{slug}-{i}")
            if stills[i]:
                # Nothing moves, so it is simply held for as long as the sentence lasts — and
                # a moment longer on the title card, before the demonstration begins.
                length = spoken + (CARD_PAUSE if i == 0 else 0.0)
                stretch = [stretch[0]] * max(1, int(length * FPS))
            else:
                # One more pass of the same pictures while there is still something to say —
                # an extra copy at a time, and never more than a few, so a long sentence does
                # not turn its clip into a carousel.
                once = list(stretch)
                for _ in range(3):
                    if len(stretch) / FPS >= spoken:
                        break
                    stretch = stretch + once
                if len(stretch) / FPS < spoken:
                    stretch = stretch + [stretch[-1]] * int((spoken - len(stretch) / FPS) * FPS)
            audio_parts.append(pad_to(spoken_audio, len(stretch) / FPS,
                                      OUT / f"_{slug}-{i}-fit.wav"))
            spoken_audio.unlink()
        else:
            audio_parts.append(silence(len(stretch) / FPS, OUT / f"_{slug}-{i}-fit.wav"))
        sequence += stretch

    # A moment at the end to take in what happened.
    sequence += [sequence[-1]] * int(TAIL_SECONDS * FPS)
    audio_parts.append(silence(TAIL_SECONDS, OUT / f"_{slug}-tail.wav"))

    for n, path in enumerate(sequence, start=1):
        subprocess.run(["cp", str(path), str(frames / f"{n:04d}.png")], check=True)
    for f in pool.glob("*.png"):
        f.unlink()
    pool.rmdir()

    audio = join_audio(audio_parts, OUT / f"_{slug}-full.m4a") if audio_parts else None
    encode(frames, OUT / f"clarity-{slug}.mp4", audio)


# In playing order, and numbered so that a folder of them sorts into that order. The narration
# blocks are keyed by the name after the number, so renumbering a section does not orphan its
# words.
# A section built from a screen recording rather than from drawn scenes: the source file, the
# seconds to take from it, and how long the excerpt runs.
SCREEN = HERE.parent.parent / "demo" / "test-gear-recording.mp4"
SCREEN_FROM = 0.0
# How long the excerpt runs on screen. The recording is a few seconds of a steady rack with
# its scopes running, so it is CROSSFADED INTO ITSELF to reach this length: a plain repeat
# jumps at the seam, and a jump reads as a fault in the video rather than as a loop.
SCREEN_LENGTH = 23.5
SCREEN_FADE = 0.7


def render_screen_section(slug, title, line):
    """The recording, with a title card in front of it and the narration timed to both.

    Frames are not extracted for this one. A minute of 1080p stills is gigabytes on disk for
    no gain, so the card and the excerpt are made as two short videos and joined.
    """
    phrases = narration_for(slug)
    work = OUT / f"_{slug}"
    work.mkdir(parents=True, exist_ok=True)

    # The card, as its own silent video.
    art.render(card(title, line), work / "card.png", width=VIDEO_W)
    card_seconds = max(CARD_SECONDS, 0.0)
    card_audio = None
    if phrases:
        spoken_audio, spoken = speak(phrases[0], f"{slug}-0")
        # Named, then a beat, then the demonstration — as in the drawn sections.
        card_seconds = max(card_seconds, spoken + CARD_PAUSE)
        card_audio = pad_to(spoken_audio, card_seconds, work / "card.wav")
        spoken_audio.unlink()
    else:
        card_audio = silence(card_seconds, work / "card.wav")
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-loop", "1",
                    "-i", str(work / "card.png"), "-t", f"{card_seconds:.3f}",
                    "-vf", "fps=30", "-c:v", "libx264", "-preset", "slow", "-crf", "18",
                    "-pix_fmt", "yuv420p",
                    "-profile:v", "high", "-level", "4.0", str(work / "card.mp4")], check=True)

    # The excerpt, scaled to the frame, its own audio dropped — the narration owns the track.
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-ss", f"{SCREEN_FROM}",
                    "-i", str(SCREEN), "-an",
                    "-vf", f"scale=-2:{VIDEO_H},pad={VIDEO_W}:{VIDEO_H}:(ow-iw)/2:0:"
                           f"color=0x161a20,fps=30",
                    "-c:v", "libx264", "-preset", "slow", "-crf", "18", "-pix_fmt", "yuv420p",
                    "-profile:v", "high", "-level", "4.0",
                    str(work / "once.mp4")], check=True)

    # Lengthened by crossfading it into itself, over and over, until it covers the words.
    have = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration",
                                 "-of", "csv=p=0", str(work / "once.mp4")],
                                capture_output=True, text=True).stdout.strip())
    current = work / "once.mp4"
    step = 0
    while have < SCREEN_LENGTH:
        step += 1
        nxt = work / f"loop{step}.mp4"
        offset = have - SCREEN_FADE
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(current),
                        "-i", str(work / "once.mp4"), "-filter_complex",
                        f"[0][1]xfade=transition=fade:duration={SCREEN_FADE}:"
                        f"offset={offset:.3f},format=yuv420p[v]",
                        "-map", "[v]", "-c:v", "libx264", "-preset", "slow", "-crf", "18",
                        str(nxt)], check=True)
        current = nxt
        have = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries",
                                     "format=duration", "-of", "csv=p=0", str(current)],
                                    capture_output=True, text=True).stdout.strip())
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(current),
                    "-t", f"{SCREEN_LENGTH}", "-c:v", "libx264", "-preset", "slow",
                    "-crf", "18", "-pix_fmt", "yuv420p",
                    "-profile:v", "high", "-level", "4.0", str(work / "screen.mp4")], check=True)

    # What is said over the recording: the rest of the phrases, one after another, then
    # silence for whatever is left of it.
    parts = [card_audio]
    spent = 0.0
    for i, phrase in enumerate(phrases[1:], start=1):
        spoken_audio, spoken = speak(phrase, f"{slug}-{i}")
        parts.append(pad_to(spoken_audio, spoken + 0.6, work / f"p{i}.wav"))
        spoken_audio.unlink()
        spent += spoken + 0.6
    if spent < SCREEN_LENGTH:
        parts.append(silence(SCREEN_LENGTH - spent, work / "rest.wav"))

    audio = join_audio(parts, work / "full.m4a")

    listing = work / "join.txt"
    listing.write_text("file 'card.mp4'\nfile 'screen.mp4'\n")
    out = OUT / f"clarity-{slug}.mp4"
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-f", "concat", "-safe", "0",
                    "-i", "join.txt", "-i", "full.m4a", "-c:v", "copy", "-c:a", "aac",
                    "-b:a", "160k", "-movflags", "+faststart", str(out)],
                   check=True, cwd=str(work))
    if not slug[0].isdigit():
        # Not a numbered section, so it is a clip in its own right rather than part of the
        # film. Named accordingly, and left where assemble() will not find it.
        renamed = OUT / "Test Gear preview.mp4"
        out.replace(renamed)
        out = renamed
    for f in work.iterdir():
        f.unlink()
    work.rmdir()
    print("wrote", out.name)


SECTIONS = [
    ("3-colour-code-jacks", "Colour code jacks", "What a jack carries, and which way it goes",
     [clips.jack_table]),
    ("4-colour-code-cables", "Colour code cables", "A cable takes the colour of where it goes",
     [clips.cable_swatches, clips.replug]),
    ("5-animate-cable-directions", "Animate cable directions",
     "Which way the signal is flowing", [clips.flow], 4),
    ("6-consistent-knob-style", "Consistent knob style", "One knob face across every plugin",
     [clips.knobs, clips.knob_rack]),
    ("7-pinch-to-zoom", "Pinch to zoom", "Two fingers, and the rack grows",
     [clips.pinch]),
    ("8-cable-trace-assist", "Cable trace assist", "Follow one lead through a tangle",
     clips.trace_parts_builders()),
    ("9-add-and-move-cables-without-dragging", "Add and move cables without dragging",
     "Click to pick up, click to put down", [clips.new_cable, clips.pull]),
    ("10-several-cables-on-one-jack", "Several cables on one jack",
     "Click again for the next one", [clips.pick_two, clips.new_from_two]),
    ("11-where-to-get-it", "Where to get it", "Free, and open source",
     [clips.download_card]),
    # The trailer: the widgets named one at a time, then the sign-off.
    ("12-test-gear-coming-soon", "Test Gear", "Thirteen widgets, coming soon",
     clips.widget_cards()),
]


def join(parts, out):
    """Sections into one film: the concat DEMUXER, re-encoding rather than copying.

    Three ways were tried and only this one is right. Copying with the demuxer trusts each
    file's own timestamps, and the AAC priming in an MP4 made the sound run to twice the
    length of the pictures. The concat FILTER re-times properly in principle, but here it
    dropped whole sections — a two minute join came out as one, with the audio timestamps
    running backwards. Re-encoding through the demuxer costs one more pass and comes out
    exactly the length of its parts, which is the only test that matters.
    """
    listing = out.with_suffix(".txt")
    listing.write_text("".join(f"file '{p.resolve()}'\n" for p in parts))
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-f", "concat", "-safe", "0",
                    "-i", str(listing),
                    "-c:v", "libx264", "-preset", "slow", "-crf", "18", "-pix_fmt", "yuv420p",
                    "-profile:v", "high", "-level", "4.0",
                    "-g", "15", "-c:a", "aac", "-b:a", "192k",
                    "-movflags", "+faststart", str(out)], check=True)
    listing.unlink()
    print("wrote", out.name, "from", len(parts), "sections")


def sections_in_order():
    def rank(path):
        return int(path.name.split("-")[1])

    return sorted((p for p in OUT.glob("clarity-*.mp4") if p.name.split("-")[1].isdigit()),
                  key=rank)


def assemble():
    """Every numbered section, in order, into one film.

    Joined with the concat FILTER rather than the demuxer. The sections were encoded one at a
    time and each carries its own AAC priming; trusting those timestamps end to end is what
    produced a soundtrack twice the length of the pictures earlier on. The filter re-times
    both streams, at the cost of one more encode.
    """
    parts = sections_in_order()
    if not parts:
        print("nothing to assemble")
        return
    # In with the sections rather than a folder above them: that is the folder anyone building
    # this has open while they work.
    join(parts, OUT / "The Clarity Module.mp4")


def assemble_full():
    """The same, with the Test Gear preview before the closing trailer.

    A second film rather than a longer one: the Clarity video is for people asking what
    Clarity does, and twenty seconds of a module they cannot have yet is a detour. This one is
    for a post about both.
    """
    parts = sections_in_order()
    preview = OUT / "Test Gear preview.mp4"
    if preview.exists():
        # Before the trailer, so the Test Gear material runs together at the end.
        trailer = [p for p in parts if p.name.split("-")[1] == "12"]
        at = parts.index(trailer[0]) if trailer else len(parts)
        parts = parts[:at] + [preview] + parts[at:]
    join(parts, OUT / "Clarity and Test Gear.mp4")


if __name__ == "__main__":
    wanted = sys.argv[1:]
    if wanted == ["assemble"]:
        assemble()
        raise SystemExit
    if wanted == ["assemble-full"]:
        assemble_full()
        raise SystemExit
    if not wanted or "title" in wanted:
        render_title()
    if not wanted or "test-gear-preview" in wanted:
        render_screen_section("test-gear-preview", "A preview of Test Gear",
                              "Thirteen widgets for any terminal")
    if not wanted or "2-a-rack-running-clarity" in wanted:
        # A moving rack rather than a photograph of one: the cables crawl, the traces run, and
        # the claim being made is about a rack in use.
        render_screen_section("2-a-rack-running-clarity", "A rack running Clarity",
                              "Every module, drawn the same way")
    for section in SECTIONS:
        slug, title, line, builders = section[:4]
        loops = section[4] if len(section) > 4 else 1
        if wanted and slug not in wanted:
            continue
        render_section(slug, title, line, builders, loops)
