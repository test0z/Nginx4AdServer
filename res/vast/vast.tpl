<VAST version="2.0">
  <Ad id="static">
    <InLine>
      <AdSystem>Mtty Tech</AdSystem>
      <AdTitle>${title}</AdTitle>
      <Impression>${impressionUrl}</Impression>
      <Creatives>
        <Creative>
          <Linear>
            <Duration>${duration}</Duration>
            <VideoClicks>
              <ClickThrough>${clickThrouth}</ClickThrough>
              <ClickTracking></ClickTracking>
            </VideoClicks>
            <MediaFiles>
              <MediaFile type="video/mp4" bitrate="${bitrate}" width="${videoWidth}" height="${videoHeight}">
                ${videoUrl}
              </MediaFile>
            </MediaFiles>
           </Linear>
        </Creative>
      </Creatives>
    </InLine>
  </Ad>
</VAST>
