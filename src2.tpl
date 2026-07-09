<%
/* stuff happens here ... */
var username = "Nobody";
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
            <% var icnt = 0; for(var key in ENV) { var val = ENV[key] %>
                <tr>
                    <td>
                        <%= key %>
                    </td>
                    <td>
                        <em>
                            <%= val %>
                        </em>
                    </td>
                </tr>
            <% icnt++; if(icnt == 5){ break; } }%>
            </tbody>
        </ul>
    </body>
</html>