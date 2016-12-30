var ca_item;
var ca_item2;
var ca_list;

onload_functions[onload_functions.length] = 'ca_resize_images();';

// resize images
function ca_resize_images()
{
    var i, limit, diff;
    limit = 600;
    diff = 225;
    ca_item = document.getElementById('contentrow');
    if(ca_item && ca_item.clientWidth)
    {
        limit = ca_item.clientWidth - diff;
    }
    if(limit < 500)
    {
        limit = 500;
    }
    if(document.body.clientWidth && document.body.clientWidth < (limit + diff) && document.body.clientWidth > 800)
    {
        limit = document.body.clientWidth - diff;
    }
    else if(window.innerWidth && window.innerWidth < (limit + diff) && window.innerWidth > 800)
    {
        limit = window.innerWidth - diff;
    }
    /* IE6 limit fix */
    if(!window.XMLHttpRequest && limit > 1500)
    {
        limit = 800;
    }
    if(ca_main_width && ca_main_width.indexOf('%') == -1)
    {
        ca_main_width.replace(/px/, '');
        if(ca_main_width > 0)
        {
            limit = ca_main_width - diff;
        }
    }
    if(ca_item)
    {
        ca_list = ca_item.getElementsByTagName('img');
    }
    else
    {
        ca_list = document.getElementsByTagName('img');
    }
    for(i=0; i<ca_list.length; i++)
    {
        ca_item = ca_list[i];
        if(ca_item.width > limit)
        {
            if(document.all) 
            { 
                ca_item.style.cursor = 'hand'; 
            }
            else
            { 
                ca_item.style.cursor = 'pointer'; 
            }
            ca_item.style.width = (limit - 50) + 'px';
            ca_item.onclick = function() { 
                window.open(this.src, 'image', 'width=700,height=500,resizable=1,scrollbars=1');
            }
        }
    }
}
