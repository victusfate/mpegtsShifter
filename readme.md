shifter - a mpegts stream file timestamp shifter

===

the plan is for this to bypass an error I'm seeing in ffmpeg for mpegts file creation, the time stamps are always 1.4sec to duration+1.4sec even after setpts filtering

usage (where 10 is offset time in seconds):
<pre>
  ./shifter input.ts 10 output.ts
</pre>

**update**
ffmpeg shifts mpegts aok at the moment, example, 10 second offset example
<pre>
ffmpeg -i in.ts -vcodec copy -acodec copy -f segment -initial_offset 10 -segment_format mpegts out%d.ts
</pre>
