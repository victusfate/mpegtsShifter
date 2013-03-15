shifter - a mpegts stream file timestamp shifter

===

the plan is for this to bypass an error I'm seeing in ffmpeg for mpegts file creation, the time stamps are always 1.4sec to duration+1.4sec even after setpts filtering

usage (where 10 is offset time in seconds):
<pre>
  ./shifter input.ts 10 output.ts
</pre>