shifter - a mpegts stream file timestamp shifter

===

the plan is for this to bypass an error I'm seeing in ffmpeg for mpegts file creation, the time stamps are always 1.4sec to duration+1.4sec even after setpts filtering

usage:
<pre>
  ./shifter (input MPEG-TS file) (offset time seconds) (output MPEG-TS file)
</pre>