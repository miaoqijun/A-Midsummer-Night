#version 330 core                                                                    
                                                                                                                                         
uniform sampler2D flame;
uniform sampler2D Round;

in vec2 TexCoord;   
in vec4 fColor;
in float fchoice;
out vec4 FragColor;                                                                 
                                                                                    
void main()                                                                         
{     
    vec4 texColor;
    if(fchoice > 0.5)
        texColor = texture(flame, TexCoord) * fColor; 
    else
        texColor = texture(Round, TexCoord) * fColor;     

    if(texColor.r + texColor.g + texColor.b < 0.2)
        discard;
    FragColor = texColor;
}
