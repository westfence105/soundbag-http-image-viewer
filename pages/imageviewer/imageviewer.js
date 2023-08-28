$(function() {
  $.ajax('/api/media/library-list', {
    type: 'get',
    dataType: 'json'
  }).then(function (data) {
    for (const f of data) {
      $("<option></option>").text(f).appendTo("#library-select");
    }
  });

  const images = [];

  const overlay = $("#overlay")
  overlay.hide();
  overlay.on('click', function () {
    overlay.hide();
  });
  const setImage = function (i) {
    const url = images[i];
    overlay.html('<img class="big-image" src="'+ url +'" /><span class="close">x</span>');
    overlay.data('image-index', i);
    $('body').addClass('no_scroll');
  };
  $('html').keydown(function (e) {
    if (e.code == 'ArrowLeft') {
      const index = overlay.data('image-index');
      console.log(index);
      if (Number.isInteger(index)) {
        if (index > 0) {
          setImage(index - 1);
        }
      }
    }
    else if (e.code == 'ArrowRight') {
      const index = overlay.data('image-index');
      console.log(index);
      if (Number.isInteger(index)) {
        if (index < images.length-1) {
          setImage(index + 1);
        }
      }
    }
  });

  const showImageGrid = function() {
    $("#main").html('<div id="container"></div>');
    for (var i = 0; i < images.length; ++i) {
      const url = images[i];
      const img = $('<img class="small-image" src="'+ url +'" />').appendTo("#container");
      img.data('index', i);
    }
    $("img.small-image").on('click', function () {
      setImage($(this).data('index'));
      overlay.show();
    });
  };

  const search = function() {
    const libname = $("#library-select").val();
    const parameters = [];
    for (const p of $("#parameters").val().split(',')) {
      parameters.push(p.trim());
    }
    $("#main").text("loading...");
    $.ajax('/api/media/search', {
      type: 'post',
      dataType: 'json',
      contentType: 'application/json',
      data: JSON.stringify({
        library: libname,
        parameters: parameters
      })
    }).then(function (data) {
      images.splice(0, images.length);
      for (const f of data) {
        images.push('/media/'+ libname +'/'+ f)
      }
      showImageGrid();
    }, function (e) {
      console.log(e);
    });
  };

  $("#search input[type='button']").on('click', search);
  $("#search input[type='text']").keydown(function (e) {
    if (e.code == 'Enter') {
      search();
    }
  });
  $("#shuffle").on('click', function () {
    for (var i = images.length - 1; 0 < i; --i) {
      const r = Math.floor(Math.random() * (i + 1));
      const tmp = images[i];
      images[i] = images[r];
      images[r] = tmp;
    }
    showImageGrid();
  });
})
