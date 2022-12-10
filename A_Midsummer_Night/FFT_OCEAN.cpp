#include "FFT_OCEAN.h"

float gaussrand()
{
    static double V1, V2, S;
    static int phase = 0;
    float X;

    if (phase == 0) {
        do {
            float U1 = (float)rand() / RAND_MAX;
            float U2 = (float)rand() / RAND_MAX;

            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while (S >= 1 || S == 0);

        X = float(V1 * sqrt(-2 * log(S) / S));
    }
    else
        X = float(V2 * sqrt(-2 * log(S) / S));

    phase = 1 - phase;

    return X;
}

float uniformRandomVariable() {
    return (float)rand() / RAND_MAX;
}

std::complex<float> gaussianRandomVariable() {
    /*float x1, x2, w;
    do {
        x1 = 2.f * uniformRandomVariable() - 1.f;
        x2 = 2.f * uniformRandomVariable() - 1.f;
        w = x1 * x1 + x2 * x2;
    } while (w >= 1.f);
    w = sqrt((-2.f * log(w)) / w);
    return std::complex<float>(x1 * w, x2 * w);*/
    float x1 = gaussrand();
    float x2 = gaussrand();
    //std::cout << x1 << " " << x2 << std::endl;
    return std::complex<float>(x1, x2);
}

cOcean::cOcean(const int N, const float A, const glm::vec2 w, const float length, const bool geometry) :
    g(9.81f), geometry(geometry), N(N), Nplus1(N + 1), A(A), w(w), length(length),
    vertices(0), indices(0), h_tilde(0), h_tilde_slopex(0), h_tilde_slopez(0), h_tilde_dx(0), h_tilde_dz(0), fft(0)
{
    h_tilde = new std::complex<float>[N * N];
    h_tilde_slopex = new std::complex<float>[N * N];
    h_tilde_slopez = new std::complex<float>[N * N];
    h_tilde_dx = new std::complex<float>[N * N];
    h_tilde_dz = new std::complex<float>[N * N];
    //fft = new cFFT(N);
    fft = new cFFT(N, 1);
    vertices = new vertex_ocean[Nplus1 * Nplus1];
    indices = new unsigned int[N * N * 6];

    int index;

    std::complex<float> htilde0, htilde0mk_conj;
    for (int m_prime = 0; m_prime < Nplus1; m_prime++) {
        for (int n_prime = 0; n_prime < Nplus1; n_prime++) {
            index = m_prime * Nplus1 + n_prime;

            htilde0 = hTilde_0(n_prime, m_prime);
            htilde0mk_conj = conj(hTilde_0(-n_prime, -m_prime));

            vertices[index].tx = float(n_prime) / Nplus1 * 0.01f;
            vertices[index].ty = float(m_prime) / Nplus1 * 0.01f;

            vertices[index].a = htilde0.real();
            vertices[index].b = htilde0.imag();
            vertices[index]._a = htilde0mk_conj.real();
            vertices[index]._b = htilde0mk_conj.imag();

            vertices[index].ox = vertices[index].x = (n_prime - N / 2.0f) * length / N;
            vertices[index].oy = vertices[index].y = 0.0f;
            vertices[index].oz = vertices[index].z = (m_prime - N / 2.0f) * length / N;

            vertices[index].nx = 0.0f;
            vertices[index].ny = 1.0f;
            vertices[index].nz = 0.0f;
        }
    }

    indices_count = 0;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * Nplus1 + n_prime;
            indices[indices_count++] = index;               // two triangles
            indices[indices_count++] = index + Nplus1;
            indices[indices_count++] = index + Nplus1 + 1;
            indices[indices_count++] = index;
            indices[indices_count++] = index + Nplus1 + 1;
            indices[indices_count++] = index + 1;

        }
    }

    ocean_shader = Shader("shaders/water_shaders/water_vs.glsl", "shaders/water_shaders/water_fs.glsl", "shaders/water_shaders/water_gs.glsl");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_vertices);
    glGenBuffers(1, &vbo_indices);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_ocean) * (Nplus1) * (Nplus1), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    stbi_set_flip_vertically_on_load(true);
    watermapTexture = loadmap(water_path, GL_MIRRORED_REPEAT);
    normalTexture = loadmap(normal_path, GL_MIRRORED_REPEAT);
    stbi_set_flip_vertically_on_load(false);
    ocean_shader.use();
    glUniform1i(glGetUniformLocation(ocean_shader.ID, "texture_albedo"), 5);
    glUniform1i(glGetUniformLocation(ocean_shader.ID, "texture_normal"), 6);
    glUniform1i(glGetUniformLocation(ocean_shader.ID, "colorMap"), 7);
    glUniform1i(glGetUniformLocation(ocean_shader.ID, "screenDepthMap"), 8);
}

cOcean::~cOcean() {
    if (h_tilde)        delete[] h_tilde;
    if (h_tilde_slopex) delete[] h_tilde_slopex;
    if (h_tilde_slopez) delete[] h_tilde_slopez;
    if (h_tilde_dx)     delete[] h_tilde_dx;
    if (h_tilde_dz)     delete[] h_tilde_dz;
    if (fft)        delete fft;
    if (vertices)       delete[] vertices;
    if (indices)        delete[] indices;
}

void cOcean::release() {
    glDeleteBuffers(1, &vbo_indices);
    glDeleteBuffers(1, &vbo_vertices);
    //releaseProgram(glProgram);
}

float cOcean::dispersion(int n_prime, int m_prime) {
    float w_0 = 2.0f * M_PI / 200.0f;
    float kx = M_PI * (2 * n_prime - N) / length;
    float kz = M_PI * (2 * m_prime - N) / length;
    return floor(sqrt(g * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
    //return sqrt(g * sqrt(kx * kx + kz * kz));
}

float cOcean::phillips(int n_prime, int m_prime) {
    glm::vec2 k(M_PI * (2 * n_prime - N) / length, M_PI * (2 * m_prime - N) / length);
    float k_length = glm::length(k);//ȡģ
    if (k_length < 0.000001) return 0.0;

    float k_length2 = k_length * k_length;
    float k_length4 = k_length2 * k_length2;

    float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(w));
    //float k_dot_w = glm::dot(k, w);
    float k_dot_w2 = k_dot_w * k_dot_w;

    float w_length = glm::length(w);
    float L = w_length * w_length / g;
    float L2 = L * L;

    //return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2;
    float damping = 0.001f;
    float l2 = L2 * damping * damping;

    return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 * exp(-k_length2 * l2);
}

std::complex<float> cOcean::hTilde_0(int n_prime, int m_prime) {
    std::complex<float> r = gaussianRandomVariable();
    return r * sqrt(phillips(n_prime, m_prime) / 2.0f);
}

std::complex<float> cOcean::hTilde(float t, int n_prime, int m_prime) {
    int index = m_prime * Nplus1 + n_prime;

    std::complex<float> htilde0(vertices[index].a, vertices[index].b);
    std::complex<float> htilde0mkconj(vertices[index]._a, vertices[index]._b);

    float omegat = dispersion(n_prime, m_prime) * t;

    float cos_ = cos(omegat);
    float sin_ = sin(omegat);

    std::complex<float> c0(cos_, sin_);
    std::complex<float> c1(cos_, -sin_);

    std::complex<float> res = htilde0 * c0 + htilde0mkconj * c1;

    return res;
}

complex_vector_normal cOcean::h_D_and_n(vector2 x, float t) {
    std::complex<float> h(0.0f, 0.0f);
    vector2 D(0.0f, 0.0f);
    vector3 n(0.0f, 0.0f, 0.0f);

    std::complex<float> c, res, htilde_c;
    vector2 k;
    float kx, kz, k_length, k_dot_x;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = 2.0f * M_PI * (m_prime - N / 2.0f) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = 2.0f * M_PI * (n_prime - N / 2.0f) / length;
            k = vector2(kx, kz);

            k_length = glm::length(k);
            k_dot_x = glm::dot(k, x);

            c = std::complex<float>(cos(k_dot_x), sin(k_dot_x));
            htilde_c = hTilde(t, n_prime, m_prime) * c;

            h = h + htilde_c;

            n = n + vector3(-kx * htilde_c.imag(), 0.0f, -kz * htilde_c.imag());

            if (k_length < 0.000001) continue;
            D = D + vector2(kx / k_length * htilde_c.imag(), kz / k_length * htilde_c.imag());
        }
    }

    n = normalize((vector3(0.0f, 1.0f, 0.0f) - n));

    complex_vector_normal cvn;
    cvn.h = h;
    cvn.D = D;
    cvn.n = n;
    return cvn;
}

void cOcean::evaluateWavesFFT(float t) {
    float kx, kz, len, lambda = -1.0f;
    int index, index1;

    for (int m_prime = 0; m_prime < N; m_prime++) {
        kz = M_PI * (2.0f * m_prime - N) / length;
        for (int n_prime = 0; n_prime < N; n_prime++) {
            kx = M_PI * (2 * n_prime - N) / length;
            len = sqrt(kx * kx + kz * kz);
            index = m_prime * N + n_prime;

            h_tilde[index] = hTilde(t, n_prime, m_prime);
            h_tilde_slopex[index] = h_tilde[index] * std::complex<float>(0, kx);
            h_tilde_slopez[index] = h_tilde[index] * std::complex<float>(0, kz);
            if (len < 0.000001f) {
                h_tilde_dx[index] = std::complex<float>(0.0f, 0.0f);
                h_tilde_dz[index] = std::complex<float>(0.0f, 0.0f);
            }
            else {
                h_tilde_dx[index] = h_tilde[index] * std::complex<float>(0, -kx / len);
                h_tilde_dz[index] = h_tilde[index] * std::complex<float>(0, -kz / len);
            }
        }
    }

    for (int m_prime = 0; m_prime < N; m_prime++) {
        fft->fft(h_tilde, 1, m_prime * N);
        fft->fft(h_tilde_slopex, 1, m_prime * N);
        fft->fft(h_tilde_slopez, 1, m_prime * N);
        fft->fft(h_tilde_dx, 1, m_prime * N);
        fft->fft(h_tilde_dz, 1, m_prime * N);
    }
    for (int n_prime = 0; n_prime < N; n_prime++) {
        fft->fft(h_tilde, N, n_prime);
        fft->fft(h_tilde_slopex, N, n_prime);
        fft->fft(h_tilde_slopez, N, n_prime);
        fft->fft(h_tilde_dx, N, n_prime);
        fft->fft(h_tilde_dz, N, n_prime);
    }

    int sign;
    float signs[] = { 1.0f, -1.0f };
    vector3 n;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * N + n_prime;     // index into h_tilde..
            index1 = m_prime * Nplus1 + n_prime;    // index into vertices
            vertices[index1].nx = 0;
            vertices[index1].ny = 0;
            vertices[index1].nz = 0;

            sign = int(signs[(n_prime + m_prime) & 1]);

            h_tilde[index] = h_tilde[index] * (float)sign;

            // height
            vertices[index1].y = stride * h_tilde[index].real();

            // displacement
            h_tilde_dx[index] = h_tilde_dx[index] * (float)sign;
            h_tilde_dz[index] = h_tilde_dz[index] * (float)sign;
            vertices[index1].x = stride * (vertices[index1].ox + h_tilde_dx[index].real() * lambda);
            vertices[index1].z = stride * (vertices[index1].oz + h_tilde_dz[index].real() * lambda);

            // normal
            h_tilde_slopex[index] = h_tilde_slopex[index] * (float)sign;
            h_tilde_slopez[index] = h_tilde_slopez[index] * (float)sign;
            n = glm::normalize(vector3(0.0f - h_tilde_slopex[index].real(), 1.0f, 0.0f - glm::normalize(h_tilde_slopez[index].real())));

            // for tiling
            if (n_prime == 0 && m_prime == 0) {
                vertices[index1 + N + Nplus1 * N].y = stride * h_tilde[index].real();

                vertices[index1 + N + Nplus1 * N].x = stride * (vertices[index1 + N + Nplus1 * N].ox + h_tilde_dx[index].real() * lambda);
                vertices[index1 + N + Nplus1 * N].z = stride * (vertices[index1 + N + Nplus1 * N].oz + h_tilde_dz[index].real() * lambda);
            }
            if (n_prime == 0) {
                vertices[index1 + N].y = stride * h_tilde[index].real();
                vertices[index1 + N].x = stride * (vertices[index1 + N].ox + h_tilde_dx[index].real() * lambda);
                vertices[index1 + N].z = stride * (vertices[index1 + N].oz + h_tilde_dz[index].real() * lambda);

            }
            if (m_prime == 0) {
                vertices[index1 + Nplus1 * N].y = stride * h_tilde[index].real();
                vertices[index1 + Nplus1 * N].x = stride * (vertices[index1 + Nplus1 * N].ox + h_tilde_dx[index].real() * lambda);
                vertices[index1 + Nplus1 * N].z = stride * (vertices[index1 + Nplus1 * N].oz + h_tilde_dz[index].real() * lambda);

            }
        }
    }

    for (int i = 0; i < N * N * 2; i++)
    {
        unsigned int pIndex1 = indices[i * 3];
        unsigned int pIndex2 = indices[i * 3 + 1];
        unsigned int pIndex3 = indices[i * 3 + 2];
        float x1 = vertices[pIndex1].x;
        float y1 = vertices[pIndex1].y;
        float z1 = vertices[pIndex1].z;
        float x2 = vertices[pIndex2].x;
        float y2 = vertices[pIndex2].y;
        float z2 = vertices[pIndex2].z;
        float x3 = vertices[pIndex3].x;
        float y3 = vertices[pIndex3].y;
        float z3 = vertices[pIndex3].z;
        //求边
        float vx1 = x2 - x1;
        float vy1 = y2 - y1;
        float vz1 = z2 - z1;
        float vx2 = x3 - x1;
        float vy2 = y3 - y1;
        float vz2 = z3 - z1;
        //叉乘求三角形法线
        float xN = vy1 * vz2 - vz1 * vy2;
        float yN = vz1 * vx2 - vx1 * vz2;
        float zN = vx1 * vy2 - vy1 * vx2;
        float Length = sqrtf(xN * xN + yN * yN + zN * zN);
        xN /= Length;
        yN /= Length;
        zN /= Length;
        //顶点法线更新
        vertices[pIndex1].nx += xN;
        vertices[pIndex1].ny += yN;
        vertices[pIndex1].nz += zN;
        vertices[pIndex2].nx += xN;
        vertices[pIndex2].ny += yN;
        vertices[pIndex2].nz += zN;
        vertices[pIndex3].nx += xN;
        vertices[pIndex3].ny += yN;
        vertices[pIndex3].nz += zN;
    }
}

void cOcean::evaluateWaves(float t) {
    float lambda = -1.0;
    int index;
    vector2 x;
    vector2 d;
    complex_vector_normal h_d_and_n;
    for (int m_prime = 0; m_prime < N; m_prime++) {
        for (int n_prime = 0; n_prime < N; n_prime++) {
            index = m_prime * Nplus1 + n_prime;

            x = vector2(vertices[index].x, vertices[index].z);

            h_d_and_n = h_D_and_n(x, t);

            vertices[index].y = h_d_and_n.h.real();

            vertices[index].x = vertices[index].ox + lambda * h_d_and_n.D.x;
            vertices[index].z = vertices[index].oz + lambda * h_d_and_n.D.y;

            vertices[index].nx = h_d_and_n.n.x;
            vertices[index].ny = h_d_and_n.n.y;
            vertices[index].nz = h_d_and_n.n.z;

            if (n_prime == 0 && m_prime == 0) {
                vertices[index + N + Nplus1 * N].y = h_d_and_n.h.real();

                vertices[index + N + Nplus1 * N].x = vertices[index + N + Nplus1 * N].ox + lambda * h_d_and_n.D.x;
                vertices[index + N + Nplus1 * N].z = vertices[index + N + Nplus1 * N].oz + lambda * h_d_and_n.D.y;

                vertices[index + N + Nplus1 * N].nx = h_d_and_n.n.x;
                vertices[index + N + Nplus1 * N].ny = h_d_and_n.n.y;
                vertices[index + N + Nplus1 * N].nz = h_d_and_n.n.z;
            }
            if (n_prime == 0) {
                vertices[index + N].y = h_d_and_n.h.real();

                vertices[index + N].x = vertices[index + N].ox + lambda * h_d_and_n.D.x;
                vertices[index + N].z = vertices[index + N].oz + lambda * h_d_and_n.D.y;

                vertices[index + N].nx = h_d_and_n.n.x;
                vertices[index + N].ny = h_d_and_n.n.y;
                vertices[index + N].nz = h_d_and_n.n.z;
            }
            if (m_prime == 0) {
                vertices[index + Nplus1 * N].y = h_d_and_n.h.real();

                vertices[index + Nplus1 * N].x = vertices[index + Nplus1 * N].ox + lambda * h_d_and_n.D.x;
                vertices[index + Nplus1 * N].z = vertices[index + Nplus1 * N].oz + lambda * h_d_and_n.D.y;

                vertices[index + Nplus1 * N].nx = h_d_and_n.n.x;
                vertices[index + Nplus1 * N].ny = h_d_and_n.n.y;
                vertices[index + Nplus1 * N].nz = h_d_and_n.n.z;
            }
        }
    }
}

void cOcean::render(float t, glm::vec3 light_pos, glm::mat4 Projection, glm::mat4 View, glm::mat4 Model, bool use_fft) {
    evaluateWavesFFT(t / 2.0f);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, watermapTexture);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    ocean_shader.setMat4("projection", Projection);
    ocean_shader.setMat4("view", View);
    ocean_shader.setVec3("light_position", light_pos);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_ocean) * Nplus1 * Nplus1, vertices);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_ocean), (void*)(6 * sizeof(GLfloat)));

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    /*glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);*/

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Model = glm::mat4(1.0f);
            Model = glm::translate(Model, position);
            Model = glm::translate(Model, glm::vec3(i * length * stride, 0, j * length * stride));
            ocean_shader.setMat4("model", Model);
            glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

unsigned int cOcean::loadmap(string path, unsigned int mode)
{
    string filename = string(path);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}