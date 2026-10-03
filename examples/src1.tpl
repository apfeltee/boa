<%
/* stuff happens here ... */
var username = "Nobody";
const items =  {
    "SHELL": "/bin/bash",
    "WSL_INTEROP": "/run/WSL/5096_interop",
    "WSL_DISTRO_NAME": "Ubuntu",
    "WT_SESSION": "<em>09bf810c-971d-4367-a27f-47adaf1366d6</em>",
};
%>
<!DOCTYPE html>
<html>
    <head>
        <title>Blah</title>
    </head>
    <body>
        <h3>Goodbye <%= username %>!</h3>
        <table>
            <thead>
                <tr>
                    <td>
                        <b>
                            Key
                        </b>
                    </td>
                </tr>
            </thead>
            <tbody>
            <% for(var key in items) { var val = items[key] %>
                <tr>
                    <td>
                        <%= Response.htmlEncode(key) %>
                    </td>
                    <td>
                        <em>
                            <%= Response.htmlEncode(val) %>
                        </em>
                    </td>
                </tr>
            <% } %>
            </tbody>
        </ul>
    </body>
</html>